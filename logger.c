/**
 ******************************************************************************
 * @file    logger.c
 * @brief   Реализация библиотеки логирования (см. logger.h).
 * @author  Mechanic
 * @date    18.09.2026
 * @version 1.2
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

static LOGGER_Record_t   s_buffer[LOGGER_BUFFER_CAPACITY];
static uint16_t          s_buffer_count;
static LOGGER_Priority_t s_max_priority;
static uint16_t          s_max_priority_count;

/** Статистика вызовов LOGGER_Mark() на каждый mark_id < LOGGER_MARK_MAX_IDS -
 *  используется только LOGGER_GetMarkFrequency(), сам LOGGER_Mark() не делает
 *  никаких вычислений с плавающей точкой (см. архитектуру в logger.h, п.8). */
typedef struct
{
    uint32_t call_count;
    uint32_t first_systick;
    uint32_t last_systick;
} logger_mark_stats_t;

static logger_mark_stats_t s_mark_stats[LOGGER_MARK_MAX_IDS];

/* ------------------------------------------------------------------------ */
/*  Служебная таблица кодов самой библиотеки (0x0000-0x00FF)                */
/* ------------------------------------------------------------------------ */

static const LOGGER_LogEntry_t s_internal_table[] =
{
    { LOGGER_INTERNAL_CODE_INIT,  LOGGER_PRIORITY_LOW, "LOGGER: инициализация выполнена" },
    { LOGGER_INTERNAL_CODE_FLUSH, LOGGER_PRIORITY_LOW, "LOGGER: буфер сброшен в память" },
    { LOGGER_INTERNAL_CODE_MARK,  LOGGER_PRIORITY_LOW, "LOGGER: временная метка" },
};

#define LOGGER_INTERNAL_TABLE_SIZE ((uint32_t)(sizeof(s_internal_table) / sizeof(s_internal_table[0])))

/* ------------------------------------------------------------------------ */
/*  Быстрый вывод в SWO (ITM) - без printf/snprintf                         */
/* ------------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------------ */
/*  Поиск записи в таблице кодов (двоичный поиск / служебная таблица)      */
/* ------------------------------------------------------------------------ */

/** Пользовательская таблица (logger_codes.h) гарантированно отсортирована по
 *  code по возрастанию, без повторов и без пересечения со служебным
 *  диапазоном - это проверяет LOGGER_Init(). Коды из служебного диапазона
 *  (0x0000-0x00FF) ищутся отдельно, линейным перебором по короткой
 *  s_internal_table (2-3 записи, быстрее и проще, чем городить общий
 *  отсортированный массив ради нескольких служебных кодов). */
static const LOGGER_LogEntry_t *logger_find_entry(uint16_t code)
{
    if (code <= LOGGER_INTERNAL_CODE_MAX)
    {
        for (uint32_t i = 0U; i < LOGGER_INTERNAL_TABLE_SIZE; i++)
        {
            if (s_internal_table[i].code == code)
            {
                return &s_internal_table[i];
            }
        }
        return NULL;
    }

    uint32_t lo = 0U;
    uint32_t hi = LOGGER_LOG_TABLE_SIZE;

    while (lo < hi)
    {
        uint32_t mid      = lo + ((hi - lo) >> 1U);
        uint16_t mid_code = LOGGER_LogTable[mid].code;

        if (mid_code == code)
        {
            return &LOGGER_LogTable[mid];
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
    return NULL;
}

/* ------------------------------------------------------------------------ */
/*  Буферизация                                                             */
/* ------------------------------------------------------------------------ */

static void logger_emit(uint16_t code, uint16_t source_id, int32_t value, uint32_t systick,
                         uint8_t persist);

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

    s_config.write_fn(s_config.write_context, (const uint8_t *)s_buffer,
                       (uint32_t)s_buffer_count * (uint32_t)sizeof(LOGGER_Record_t));

    s_buffer_count       = 0U;
    s_max_priority_count = 0U;

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
    const LOGGER_LogEntry_t *entry = logger_find_entry(code);

    LOGGER_Priority_t priority    = (entry != NULL) ? entry->priority    : LOGGER_PRIORITY_HIGH;
    const char       *description = (entry != NULL) ? entry->description : NULL;
    uint32_t          rtc_time    = (s_config.rtc_time_fn != NULL)
                                     ? s_config.rtc_time_fn(s_config.rtc_context) : 0U;

    if (s_config.console_fn != NULL)
    {
        s_config.console_fn(s_config.console_context, code, source_id, priority, value,
                             systick, rtc_time, description);
    }
    else
    {
        logger_swo_output(code, source_id, priority, value, systick, rtc_time, description);
    }

    if ((persist != 0U) && s_buffering_active && (entry != NULL))
    {
        logger_buffer_push(entry->code, priority, source_id, value, systick, rtc_time);
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
    uint32_t now = HAL_GetTick();

    if (mark_id < LOGGER_MARK_MAX_IDS)
    {
        logger_mark_stats_t *stats = &s_mark_stats[mark_id];

        if (stats->call_count == 0U)
        {
            stats->first_systick = now;
        }
        stats->last_systick = now;
        stats->call_count++;
    }

    logger_emit(LOGGER_INTERNAL_CODE_MARK, mark_id, (int32_t)mark_id, now, 1U);
}

HAL_StatusTypeDef LOGGER_GetMarkFrequency(uint16_t mark_id, float *out_frequency)
{
    if ((out_frequency == NULL) || (mark_id >= LOGGER_MARK_MAX_IDS))
    {
        return HAL_ERROR;
    }

    const logger_mark_stats_t *stats = &s_mark_stats[mark_id];

    if (stats->call_count < 2U)
    {
        return HAL_ERROR; /* недостаточно вызовов, чтобы иметь интервал для расчёта */
    }

    /* Беззнаковая разность корректно работает и при однократном переполнении
     * HAL_GetTick() между первым и последним вызовом метки. */
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
