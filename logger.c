/**
 ******************************************************************************
 * @file    logger.c
 * @brief   Реализация библиотеки логирования (см. logger.h).
 * @author  Mechanic
 * @date    19.09.2026
 * @version 1.7
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

#include "logger.h"
#include "logger_codes.h"
#include <stddef.h>

/* ------------------------------------------------------------------------ */
/*  Состояние модуля (статика, без malloc)                                  */
/* ------------------------------------------------------------------------ */

/* Все статические переменные обнуляются компилятором при старте (BSS), это
 * само по себе даёт корректный "режим без инициализации": s_config.console_fn
 * и s_config.write_fn равны NULL, s_buffering_active равен 0 - LOGGER_Log()
 * до вызова LOGGER_Init() работает в минимальном режиме "только вывод в SWO",
 * как описано в шапке logger.h. */

static LOGGER_Config_t   s_config;
static uint8_t           s_buffering_active;
static uint8_t           s_threshold[LOGGER_PRIORITY_COUNT];

static LOGGER_BUFFER_SECTION_ATTR LOGGER_Record_t s_buffer[LOGGER_BUFFER_CAPACITY];
static uint16_t          s_buffer_count;
static LOGGER_Priority_t s_max_priority;
static uint16_t          s_max_priority_count;

/** Статистика вызовов - на каждый mark_id < LOGGER_MARK_MAX_IDS (см.
 *  LOGGER_GetMarkFrequency()) и, отдельно, на каждый код из пользовательской
 *  таблицы (см. s_code_stats/LOGGER_GetCodeFrequency() ниже). Ни
 *  LOGGER_Mark(), ни LOGGER_Log() не делают вычислений с плавающей точкой на
 *  горячем пути - только обновляют эти счётчики, деление - по явному запросу
 *  в LOGGER_Get{Mark,Code}Frequency() (см. архитектуру в logger.h, п.8/9). */
typedef struct
{
    uint32_t call_count;
    uint32_t first_systick;
    uint32_t last_systick;
} logger_freq_stats_t;

static logger_freq_stats_t s_mark_stats[LOGGER_MARK_MAX_IDS];

/** Статистика частоты вызова обычных кодов из пользовательской таблицы -
 *  индекс совпадает с индексом кода в LOGGER_LogTable (logger_codes.h), см.
 *  logger_find_user_entry_index(). */
static logger_freq_stats_t s_code_stats[LOGGER_LOG_TABLE_SIZE];

/* ------------------------------------------------------------------------ */
/*  Более точный источник времени статистики меток (опционально, DWT)       */
/* ------------------------------------------------------------------------ */

#if LOGGER_MARK_TIME_SOURCE != LOGGER_MARK_TIME_SYSTICK_MS
/** Сколько тактов ядра (DWT->CYCCNT) приходится на одну единицу измерения
 *  статистики меток (мс или мкс, см. LOGGER_MARK_TIME_SOURCE) - вычисляется
 *  один раз в LOGGER_Init() по текущей частоте HCLK. */
static uint32_t s_mark_dwt_cycles_per_unit = 1U;

/** Включает аппаратный счётчик тактов ядра DWT->CYCCNT и пересчитывает
 *  делитель для перевода тактов в единицы измерения статистики меток.
 *  Безопасно вызывать повторно (например, при реинициализации). */
static void logger_mark_dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    uint32_t hclk_hz = HAL_RCC_GetHCLKFreq();
#if LOGGER_MARK_TIME_SOURCE == LOGGER_MARK_TIME_DWT_US
    uint32_t cycles_per_unit = hclk_hz / 1000000U;
#else
    uint32_t cycles_per_unit = hclk_hz / 1000U;
#endif
    s_mark_dwt_cycles_per_unit = (cycles_per_unit != 0U) ? cycles_per_unit : 1U;
}
#endif /* LOGGER_MARK_TIME_SOURCE != LOGGER_MARK_TIME_SYSTICK_MS */

/** Текущее время для статистики LOGGER_Mark()/LOGGER_GetMarkFrequency() в
 *  единицах, заданных LOGGER_MARK_TIME_SOURCE (мс через SysTick по умолчанию,
 *  либо мс/мкс через DWT->CYCCNT). НЕ используется для systick, попадающего в
 *  сами записи логов/буфер - тот всегда HAL_GetTick(). */
static uint32_t logger_mark_now(void)
{
#if LOGGER_MARK_TIME_SOURCE == LOGGER_MARK_TIME_SYSTICK_MS
    return HAL_GetTick();
#else
    return DWT->CYCCNT / s_mark_dwt_cycles_per_unit;
#endif
}

/** Множитель для перевода "вызовов за интервал" в "вызовов в секунду" -
 *  зависит от единицы измерения статистики меток (1000 для мс, 1000000 для
 *  мкс), см. LOGGER_MARK_TIME_SOURCE. */
#if LOGGER_MARK_TIME_SOURCE == LOGGER_MARK_TIME_DWT_US
#define LOGGER_MARK_HZ_NUMERATOR 1000000.0f
#else
#define LOGGER_MARK_HZ_NUMERATOR 1000.0f
#endif

/* ------------------------------------------------------------------------ */
/*  Быстрый вывод в SWO (ITM) - без printf/snprintf                         */
/*  Весь этот раздел исключается из сборки, если определён LOGGER_NO_ITM    */
/*  (ядра без блока ITM - Cortex-M0/M0+, см. README.md) - на таких ядрах    */
/*  console_fn становится обязательной.                                    */
/* ------------------------------------------------------------------------ */

#ifndef LOGGER_NO_ITM

static void logger_swo_putc(char c)
{
    (void)ITM_SendChar((uint32_t)c);
}

static void logger_swo_puts(const char *s)
{
    while (*s != '\0')
    {
        logger_swo_putc(*s);
        s++;
    }
}

/** Печатает 16-битное число в HEX, ровно 4 символа, без ведущего "0x". */
static void logger_swo_put_hex16(uint16_t value)
{
    static const char hex_digits[] = "0123456789ABCDEF"; /* 17 байт с '\0', не используется как строка */
    char buf[4];

    for (int8_t i = 3; i >= 0; i--)
    {
        buf[i] = hex_digits[value & 0xFU];
        value = (uint16_t)(value >> 4);
    }
    for (uint8_t i = 0U; i < 4U; i++)
    {
        logger_swo_putc(buf[i]);
    }
}

/** Печатает беззнаковое 32-битное число в десятичном виде, без ведущих нулей. */
static void logger_swo_put_uint32(uint32_t value)
{
    char buf[10]; /* максимум 10 цифр у 4294967295 */
    uint8_t idx = 0U;

    if (value == 0U)
    {
        buf[idx] = '0';
        idx++;
    }
    else
    {
        while (value != 0U)
        {
            buf[idx] = (char)('0' + (value % 10U));
            idx++;
            value /= 10U;
        }
    }

    while (idx > 0U)
    {
        idx--;
        logger_swo_putc(buf[idx]);
    }
}

/** Печатает знаковое 32-битное число в десятичном виде. */
static void logger_swo_put_int32(int32_t value)
{
    if (value < 0)
    {
        logger_swo_putc('-');
        /* аккуратно с INT32_MIN - его положительной пары не существует в int32_t */
        logger_swo_put_uint32((uint32_t)(-(value + 1)) + 1U);
    }
    else
    {
        logger_swo_put_uint32((uint32_t)value);
    }
}

static const char *logger_priority_name(LOGGER_Priority_t priority)
{
    switch (priority)
    {
        case LOGGER_PRIORITY_LOW:    return "LOW ";
        case LOGGER_PRIORITY_MEDIUM: return "MED ";
        case LOGGER_PRIORITY_HIGH:   return "HIGH";
        default:                     return "??? ";
    }
}

/** Вывод по умолчанию - используется, если console_fn не задана в LOGGER_Init(). */
static void logger_swo_output(uint16_t code, uint16_t source_id, LOGGER_Priority_t priority,
                               int32_t value, uint32_t systick, uint32_t rtc_time,
                               const char *description)
{
    logger_swo_puts("[LOG] 0x");
    logger_swo_put_hex16(code);
    logger_swo_puts(" src=0x");
    logger_swo_put_hex16(source_id);
    logger_swo_puts(" [");
    logger_swo_puts(logger_priority_name(priority));
    logger_swo_puts("] val=");
    logger_swo_put_int32(value);
    logger_swo_puts(" t=");
    logger_swo_put_uint32(systick);
    logger_swo_puts("ms rtc=");
    logger_swo_put_uint32(rtc_time);
    logger_swo_puts(" : ");
    logger_swo_puts((description != NULL) ? description : "???");
    logger_swo_puts("\r\n");
}

#endif /* !LOGGER_NO_ITM */

/* ------------------------------------------------------------------------ */
/*  Поиск записи в таблице кодов (двоичный поиск / служебная таблица)      */
/* ------------------------------------------------------------------------ */

/** Пользовательская таблица (logger_codes.h) гарантированно отсортирована по
 *  code по возрастанию, без повторов и без пересечения со служебным
 *  диапазоном - это проверяет LOGGER_Init(). Коды из служебного диапазона
 *  (0x0000-0x00FF) ищутся отдельно, линейным перебором по короткой
 *  LOGGER_InternalTable (2-3 записи, см. logger_types.h - быстрее и проще,
 *  чем городить общий отсортированный массив ради нескольких служебных
 *  кодов; та же таблица используется и хостовым декодером). */
static const LOGGER_LogEntry_t *logger_find_internal_entry(uint16_t code)
{
    for (uint32_t i = 0U; i < LOGGER_INTERNAL_TABLE_SIZE; i++)
    {
        if (LOGGER_InternalTable[i].code == code)
        {
            return &LOGGER_InternalTable[i];
        }
    }
    return NULL;
}

/** Двоичный поиск кода в пользовательской таблице (logger_codes.h) -
 *  возвращает ИНДЕКС записи (не указатель), чтобы этим же индексом сразу
 *  обновить статистику частоты кода в s_code_stats (см. logger_emit()).
 *  code гарантированно > LOGGER_INTERNAL_CODE_MAX у вызывающей стороны.
 * @retval индекс в LOGGER_LogTable; LOGGER_LOG_TABLE_SIZE, если не найден */
static uint32_t logger_find_user_entry_index(uint16_t code)
{
    uint32_t lo = 0U;
    uint32_t hi = LOGGER_LOG_TABLE_SIZE;

    while (lo < hi)
    {
        uint32_t mid      = lo + ((hi - lo) >> 1U);
        uint16_t mid_code = LOGGER_LogTable[mid].code;

        if (mid_code == code)
        {
            return mid;
        }
        if (mid_code < code)
        {
            lo = mid + 1U;
        }
        else
        {
            hi = mid;
        }
    }
    return LOGGER_LOG_TABLE_SIZE; /* не найден */
}

/* ------------------------------------------------------------------------ */
/*  Буферизация                                                             */
/* ------------------------------------------------------------------------ */

static void logger_emit(uint16_t code, uint16_t source_id, int32_t value, uint32_t systick,
                         uint8_t persist);

/** Собственно копирует буфер в write_fn и обнуляет счётчики - без каких-либо
 *  побочных действий (без служебного лога о сбросе). Вызывается только когда
 *  s_buffering_active != 0, то есть write_fn гарантированно не NULL и буфер
 *  не пуст (проверяется вызывающей стороной). Отдельная функция нужна, чтобы
 *  LOGGER_EmergencySave() мог сохранить буфер в память МИНИМАЛЬНЫМ числом
 *  действий, не тратя время на вывод служебного лога о сбросе (см. её
 *  комментарий) - на аварийном пути (скорая перезагрузка/потеря питания)
 *  каждая лишняя операция - это риск не успеть записать данные. */
static void logger_flush_raw(void)
{
    uint16_t flushed_count = s_buffer_count;

    s_config.write_fn(s_config.write_context, (const uint8_t *)s_buffer,
                       (uint32_t)flushed_count * (uint32_t)sizeof(LOGGER_Record_t));

    s_buffer_count       = 0U;
    s_max_priority_count = 0U;
}

/** Сбрасывает накопленный буфер в write_fn (если он задан и буфер не пуст) и
 *  обнуляет счётчики. Вызывается только когда s_buffering_active != 0, то
 *  есть write_fn гарантированно не NULL. После записи логирует служебное
 *  событие LOGGER_INTERNAL_CODE_FLUSH - ТОЛЬКО выводом (persist=0), чтобы не
 *  порождать рекурсивный сброс буфера о самом себе. */
static void logger_flush_internal(void)
{
    if (s_buffer_count == 0U)
    {
        return;
    }

    uint16_t flushed_count = s_buffer_count;

    logger_flush_raw();

    logger_emit(LOGGER_INTERNAL_CODE_FLUSH, 0U, (int32_t)flushed_count, HAL_GetTick(), 0U);
}

/** Добавляет запись в буфер и, при достижении настроенного порога по текущему
 *  максимальному приоритету буфера, инициирует сброс в память. O(1). */
static void logger_buffer_push(uint16_t code, LOGGER_Priority_t priority, uint16_t source_id,
                                int32_t value, uint32_t systick, uint32_t rtc_time)
{
    if (s_buffer_count >= (uint16_t)LOGGER_BUFFER_CAPACITY)
    {
        /* Физическая защита буфера - не должна срабатывать при корректно
         * настроенных порогах (см. LOGGER_Init), но не даёт выйти за границы
         * массива при любых обстоятельствах. */
        logger_flush_internal();
    }

    s_buffer[s_buffer_count].code      = code;
    s_buffer[s_buffer_count].source_id = source_id;
    s_buffer[s_buffer_count].value     = value;
    s_buffer[s_buffer_count].systick   = systick;
    s_buffer[s_buffer_count].rtc_time  = rtc_time;
    s_buffer_count++;

    if ((s_max_priority_count == 0U) || (priority > s_max_priority))
    {
        s_max_priority       = priority;
        s_max_priority_count = 1U;
    }
    else if (priority == s_max_priority)
    {
        s_max_priority_count++;
    }
    /* priority < s_max_priority: запись заняла место в буфере, но не
     * учитывается в счётчике триггера сброса - см. архитектуру в logger.h */

    if (s_max_priority_count >= s_threshold[s_max_priority])
    {
        logger_flush_internal();
    }
}

/* ------------------------------------------------------------------------ */
/*  Общая статистика частоты вызовов (меток и обычных кодов)                */
/* ------------------------------------------------------------------------ */

/** Обновляет call_count/first_systick/last_systick одной записи статистики -
 *  общая логика для s_mark_stats (LOGGER_Mark) и s_code_stats (LOGGER_Log). */
static void logger_freq_stats_bump(logger_freq_stats_t *stats, uint32_t now)
{
    if (stats->call_count == 0U)
    {
        stats->first_systick = now;
    }
    stats->last_systick = now;
    stats->call_count++;
}

/* ------------------------------------------------------------------------ */
/*  Общий внутренний конвейер вывода + (опционально) буферизации            */
/* ------------------------------------------------------------------------ */

/** Общая реализация для LOGGER_Log()/LOGGER_Mark()/служебных логов -
 *  принимает уже захваченный systick (чтобы не звать HAL_GetTick() дважды,
 *  когда вызывающая сторона его уже считала - см. LOGGER_Mark()). persist=0
 *  используется только служебными логами про саму библиотеку (INIT/FLUSH),
 *  которые не должны попадать в буфер (см. logger.h, п.6). */
static void logger_emit(uint16_t code, uint16_t source_id, int32_t value, uint32_t systick,
                         uint8_t persist)
{
    const LOGGER_LogEntry_t *entry             = NULL;
    uint32_t                 user_table_index  = LOGGER_LOG_TABLE_SIZE; /* сентинел "не код пользователя" */

    if (code <= LOGGER_INTERNAL_CODE_MAX)
    {
        entry = logger_find_internal_entry(code);
    }
    else
    {
        user_table_index = logger_find_user_entry_index(code);
        if (user_table_index < LOGGER_LOG_TABLE_SIZE)
        {
            entry = &LOGGER_LogTable[user_table_index];
        }
    }

    LOGGER_Priority_t priority    = (entry != NULL) ? entry->priority    : LOGGER_PRIORITY_HIGH;
    const char       *description = (entry != NULL) ? entry->description : NULL;
    uint32_t          rtc_time    = (s_config.rtc_time_fn != NULL)
                                     ? s_config.rtc_time_fn(s_config.rtc_context) : 0U;

    if (s_config.console_fn != NULL)
    {
        s_config.console_fn(s_config.console_context, code, source_id, priority, value,
                             systick, rtc_time, description);
    }
#ifndef LOGGER_NO_ITM
    else
    {
        logger_swo_output(code, source_id, priority, value, systick, rtc_time, description);
    }
#endif

    if ((persist != 0U) && s_buffering_active && (entry != NULL))
    {
        logger_buffer_push(entry->code, priority, source_id, value, systick, rtc_time);
    }

    /* Статистика "болтливости" кода - для ЛЮБОГО известного кода пользователя,
     * независимо от буферизации (см. LOGGER_GetCodeFrequency() в logger.h). */
    if (user_table_index < LOGGER_LOG_TABLE_SIZE)
    {
        logger_freq_stats_bump(&s_code_stats[user_table_index], systick);
    }
}

/* ------------------------------------------------------------------------ */
/*  Инициализация                                                           */
/* ------------------------------------------------------------------------ */

HAL_StatusTypeDef LOGGER_Init(const LOGGER_Config_t *config)
{
    if (config == NULL)
    {
        return HAL_ERROR;
    }

    /* Проверка таблицы кодов - один раз, здесь, чтобы LOGGER_Log() мог
     * доверять её корректности и использовать быстрый двоичный поиск без
     * дополнительных проверок в рантайме. */
    if (LOGGER_LOG_TABLE_SIZE == 0U)
    {
        return HAL_ERROR;
    }

    for (uint32_t i = 0U; i < LOGGER_LOG_TABLE_SIZE; i++)
    {
        const LOGGER_LogEntry_t *entry = &LOGGER_LogTable[i];

        if (entry->code <= LOGGER_INTERNAL_CODE_MAX)
        {
            return HAL_ERROR; /* код из зарезервированного служебного диапазона 0x0000-0x00FF */
        }

        if ((entry->priority != LOGGER_PRIORITY_LOW) &&
            (entry->priority != LOGGER_PRIORITY_MEDIUM) &&
            (entry->priority != LOGGER_PRIORITY_HIGH))
        {
            return HAL_ERROR; /* некорректный приоритет в таблице */
        }

        if (entry->description == NULL)
        {
            return HAL_ERROR;
        }

        uint32_t len = 0U;
        while ((len <= LOGGER_MAX_DESCRIPTION_LENGTH) && (entry->description[len] != '\0'))
        {
            len++;
        }
        if (len > LOGGER_MAX_DESCRIPTION_LENGTH)
        {
            return HAL_ERROR; /* описание длиннее LOGGER_MAX_DESCRIPTION_LENGTH */
        }

        if ((i > 0U) && (entry->code <= LOGGER_LogTable[i - 1U].code))
        {
            return HAL_ERROR; /* таблица не отсортирована по возрастанию либо есть повтор кода */
        }
    }

    if (config->write_fn != NULL)
    {
        if ((config->threshold_low == 0U)    || (config->threshold_low    > LOGGER_BUFFER_CAPACITY) ||
            (config->threshold_medium == 0U) || (config->threshold_medium > LOGGER_BUFFER_CAPACITY) ||
            (config->threshold_high == 0U)   || (config->threshold_high   > LOGGER_BUFFER_CAPACITY))
        {
            return HAL_ERROR;
        }
    }

#ifdef LOGGER_NO_ITM
    /* На ядрах без ITM (LOGGER_NO_ITM определён - см. README.md) вывода по
     * умолчанию не существует вовсе - console_fn обязательна, иначе логи
     * будут просто молча теряться. */
    if (config->console_fn == NULL)
    {
        return HAL_ERROR;
    }
#endif

#if LOGGER_MARK_TIME_SOURCE != LOGGER_MARK_TIME_SYSTICK_MS
    logger_mark_dwt_init();
#endif

    /* Реинициализация: сначала сбрасываем в память то, что уже накопил
     * предыдущий буфер, и только потом применяем новую конфигурацию. */
    if (s_buffering_active)
    {
        logger_flush_internal();
    }

    s_config = *config;
    s_threshold[LOGGER_PRIORITY_LOW]    = config->threshold_low;
    s_threshold[LOGGER_PRIORITY_MEDIUM] = config->threshold_medium;
    s_threshold[LOGGER_PRIORITY_HIGH]   = config->threshold_high;

    s_buffering_active    = (config->write_fn != NULL) ? 1U : 0U;
    s_buffer_count        = 0U;
    s_max_priority_count  = 0U;
    s_max_priority        = LOGGER_PRIORITY_LOW;

    logger_emit(LOGGER_INTERNAL_CODE_INIT, 0U, (int32_t)LOGGER_LOG_TABLE_SIZE, HAL_GetTick(), 0U);

    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Приём лога - главная горячая функция                                    */
/* ------------------------------------------------------------------------ */

void LOGGER_Log(uint16_t code, uint16_t source_id, int32_t value)
{
    logger_emit(code, source_id, value, HAL_GetTick(), 1U);
}

/* ------------------------------------------------------------------------ */
/*  Временные метки и их частота                                            */
/* ------------------------------------------------------------------------ */

void LOGGER_Mark(uint16_t mark_id)
{
    uint32_t now = HAL_GetTick(); /* попадает в саму запись лога/буфер - всегда мс */

    if (mark_id < LOGGER_MARK_MAX_IDS)
    {
        /* Статистика частоты метки - в единицах LOGGER_MARK_TIME_SOURCE
         * (мс через SysTick по умолчанию, либо мс/мкс через DWT), НЕ путать
         * с 'now' выше, который идёт в саму запись лога. */
        logger_freq_stats_bump(&s_mark_stats[mark_id], logger_mark_now());
    }

    logger_emit(LOGGER_INTERNAL_CODE_MARK, mark_id, (int32_t)mark_id, now, 1U);
}

HAL_StatusTypeDef LOGGER_GetMarkFrequency(uint16_t mark_id, float *out_frequency)
{
    if ((out_frequency == NULL) || (mark_id >= LOGGER_MARK_MAX_IDS))
    {
        return HAL_ERROR;
    }

    const logger_freq_stats_t *stats = &s_mark_stats[mark_id];

    if (stats->call_count < 2U)
    {
        return HAL_ERROR; /* недостаточно вызовов, чтобы иметь интервал для расчёта */
    }

    /* Беззнаковая разность корректно работает и при однократном переполнении
     * счётчика (SysTick или DWT->CYCCNT) между первым и последним вызовом. */
    uint32_t elapsed = stats->last_systick - stats->first_systick;
    if (elapsed == 0U)
    {
        return HAL_ERROR; /* все вызовы попали в один и тот же тик источника времени */
    }

    *out_frequency = ((float)(stats->call_count - 1U) * LOGGER_MARK_HZ_NUMERATOR) / (float)elapsed;
    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Статистика частоты обычных кодов ("болтливые" коды логов)               */
/* ------------------------------------------------------------------------ */

HAL_StatusTypeDef LOGGER_GetCodeFrequency(uint16_t code, float *out_frequency)
{
    if (out_frequency == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t idx = logger_find_user_entry_index(code);
    if (idx >= LOGGER_LOG_TABLE_SIZE)
    {
        return HAL_ERROR; /* код не найден в пользовательской таблице */
    }

    const logger_freq_stats_t *stats = &s_code_stats[idx];

    if (stats->call_count < 2U)
    {
        return HAL_ERROR; /* недостаточно вызовов, чтобы иметь интервал для расчёта */
    }

    uint32_t elapsed_ms = stats->last_systick - stats->first_systick;
    if (elapsed_ms == 0U)
    {
        return HAL_ERROR; /* все вызовы попали в один и тот же миллисекундный тик */
    }

    *out_frequency = ((float)(stats->call_count - 1U) * 1000.0f) / (float)elapsed_ms;
    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Принудительный сброс буфера                                             */
/* ------------------------------------------------------------------------ */

HAL_StatusTypeDef LOGGER_Flush(void)
{
    if (!s_buffering_active)
    {
        return HAL_ERROR;
    }
    logger_flush_internal();
    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Аварийное сохранение (сброс перед потерей питания/перезагрузкой)        */
/* ------------------------------------------------------------------------ */

HAL_StatusTypeDef LOGGER_EmergencySave(void)
{
    if (!s_buffering_active)
    {
        return HAL_ERROR;
    }
    if (s_buffer_count == 0U)
    {
        return HAL_OK;
    }
    logger_flush_raw();
    return HAL_OK;
}
