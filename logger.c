/**
 ******************************************************************************
 * @file    logger.c
 * @brief   Реализация библиотеки логирования (см. logger.h).
 * @author  Mechanic
 * @date    04.10.2026
 * @version 1.16
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

#include "logger.h"
#include "logger_codes.h"
#include <stddef.h>
#include <string.h>

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
/** s_buffer - КОЛЬЦО: s_buffer_head - индекс самой старой записи,
 *  s_buffer_count - сколько записей в кольце (включая те, что прямо сейчас
 *  уходят в write_fn). Пока идёт write_fn, новые записи пишутся в свободную
 *  часть кольца - сброс не блокирует приём. */
static uint16_t          s_buffer_head;
static uint16_t          s_buffer_count;
/** Счётчик записей, которые не удалось сохранить (кольцо полно и освободить
 *  место не вышло) - с момента старта, см. LOGGER_GetDroppedCount(). */
static uint32_t          s_dropped_count;
/** Пик заполнения кольца - см. LOGGER_GetBufferHighWater(). */
static uint16_t          s_high_water;
/** Флаг "сброс уже идёт" - делает logger_flush_raw() эксклюзивным между
 *  конкурентными вызовами (см. комментарий перед функцией: без этого флага
 *  два одновременных сброса из разных контекстов вычитают flushed_count из
 *  s_buffer_count дважды - второе вычитание уводит счётчик в underflow. */
static uint8_t           s_flush_in_progress;
/** LOGGER_Init() успешно выполнялся хотя бы раз. */
static uint8_t           s_initialized;
/** Потеряно записей с момента последней сохранённой сводки
 *  LOGGER_INTERNAL_CODE_DROPPED_SUMMARY (см. logger_summary_put_locked()). */
static uint32_t          s_dropped_unreported;

/** Ошибки самой библиотеки (см. logger_report_error()): по виду ошибки -
 *  счётчик срабатываний с момента старта и служебный код. Вид = индекс. */
typedef enum
{
    LOGGER_ERR_OVERFLOW = 0,
    LOGGER_ERR_UNKNOWN_CODE,
    LOGGER_ERR_RING_CORRUPT,
    LOGGER_ERR_WAIT_TIMEOUT,
    LOGGER_ERR_MARK_RANGE,
    LOGGER_ERR_NOT_INIT,
    LOGGER_ERR_COUNT
} logger_err_kind_t;

static const uint16_t s_err_codes[LOGGER_ERR_COUNT] =
{
    LOGGER_INTERNAL_CODE_BUFFER_OVERFLOW,
    LOGGER_INTERNAL_CODE_UNKNOWN_CODE,
    LOGGER_INTERNAL_CODE_RING_CORRUPT,
    LOGGER_INTERNAL_CODE_WAIT_TIMEOUT,
    LOGGER_INTERNAL_CODE_MARK_ID_RANGE,
    LOGGER_INTERNAL_CODE_NOT_INIT
};

static uint32_t s_err_count[LOGGER_ERR_COUNT];
/** 1, пока выводится служебная запись об ошибке: защита от рекурсии - ошибка,
 *  возникшая при выводе ошибки (например, переполнение при сохранении
 *  записи об ошибке), только считается и новой записи не порождает. */
static uint8_t  s_reporting;
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

/** @brief Включает DWT->CYCCNT и пересчитывает делитель тактов в единицы
 *         измерения статистики меток. Безопасно вызывать повторно. */
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

/** @brief Текущее время для статистики меток, в единицах LOGGER_MARK_TIME_SOURCE.
 *         НЕ используется для systick самих записей лога (тот всегда HAL_GetTick()).
 * @return время в единицах, заданных LOGGER_MARK_TIME_SOURCE */
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
/*  Форматирование строки лога - без printf/snprintf. Один и тот же формат  */
/*  используется и для SWO (ITM, по символу), и для LOGGER_FormatConsoleLine */
/*  (в буфер). Под LOGGER_NO_ITM (ядра без блока ITM - Cortex-M0/M0+, см.   */
/*  README.md) вывод в ITM исключается из сборки - console_fn обязательна,  */
/*  а форматтер в буфер остаётся доступным.                                 */
/* ------------------------------------------------------------------------ */

/** Приёмник символов: to_buf != 0 - запись в buf (с учётом size; buf может
 *  быть NULL - тогда только подсчёт длины); to_buf == 0 - вывод в ITM.
 *  len считает ВСЕ символы, как snprintf. */
typedef struct
{
    char    *buf;
    size_t   size;
    size_t   len;
    uint8_t  to_buf;
} logger_out_t;

/** @brief Принимает один символ: в буфер (если задан и есть место под
 *         символ и завершающий '\0') либо в SWO (ITM).
 * @param  o приёмник
 * @param  c символ для вывода */
static void logger_out_putc(logger_out_t *o, char c)
{
    if (o->to_buf != 0U)
    {
        if ((o->buf != NULL) && ((o->len + 1U) < o->size))
        {
            o->buf[o->len] = c;
        }
    }
#ifndef LOGGER_NO_ITM
    else
    {
        (void)ITM_SendChar((uint32_t)c);
    }
#endif
    o->len++;
}

/** @brief Выводит строку посимвольно.
 * @param  o приёмник
 * @param  s строка, завершённая '\0' */
static void logger_out_puts(logger_out_t *o, const char *s)
{
    while (*s != '\0')
    {
        logger_out_putc(o, *s);
        s++;
    }
}

/** @brief Печатает 16-битное число в HEX, ровно 4 символа, без "0x".
 * @param  o     приёмник
 * @param  value число для вывода */
static void logger_out_hex16(logger_out_t *o, uint16_t value)
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
        logger_out_putc(o, buf[i]);
    }
}

/** @brief Печатает десятичное число, выровненное вправо пробелами до width
 *         (если число длиннее - без обрезки).
 * @param  o         приёмник
 * @param  magnitude модуль числа
 * @param  negative  1 - перед числом знак '-'
 * @param  width     минимальная ширина поля */
static void logger_out_dec(logger_out_t *o, uint32_t magnitude, uint8_t negative, uint8_t width)
{
    char buf[10]; /* максимум 10 цифр у 4294967295 */
    uint8_t idx = 0U;

    if (magnitude == 0U)
    {
        buf[idx] = '0';
        idx++;
    }
    else
    {
        while (magnitude != 0U)
        {
            buf[idx] = (char)('0' + (magnitude % 10U));
            idx++;
            magnitude /= 10U;
        }
    }

    uint8_t used = (uint8_t)(idx + negative);
    for (uint8_t i = used; i < width; i++)
    {
        logger_out_putc(o, ' ');
    }
    if (negative != 0U)
    {
        logger_out_putc(o, '-');
    }
    while (idx > 0U)
    {
        idx--;
        logger_out_putc(o, buf[idx]);
    }
}

/** @brief Беззнаковое 32-битное число, выровнено вправо до width. */
static void logger_out_uint32(logger_out_t *o, uint32_t value, uint8_t width)
{
    logger_out_dec(o, value, 0U, width);
}

/** @brief Знаковое 32-битное число, выровнено вправо до width. */
static void logger_out_int32(logger_out_t *o, int32_t value, uint8_t width)
{
    if (value < 0)
    {
        /* аккуратно с INT32_MIN - его положительной пары не существует в int32_t */
        logger_out_dec(o, (uint32_t)(-(value + 1)) + 1U, 1U, width);
    }
    else
    {
        logger_out_dec(o, (uint32_t)value, 0U, width);
    }
}

/** @brief Одна буква приоритета: L / M / H (либо '?'). */
static char logger_priority_letter(LOGGER_Priority_t priority)
{
    switch (priority)
    {
        case LOGGER_PRIORITY_LOW:    return 'L';
        case LOGGER_PRIORITY_MEDIUM: return 'M';
        case LOGGER_PRIORITY_HIGH:   return 'H';
        default:                     return '?';
    }
}

/** @brief Формирует строку лога в приёмник - единый формат SWO и
 *         LOGGER_FormatConsoleLine() (колонки фиксированной ширины):
 *         "<RTC,19> <мс,8> <ГРУППА,8> <L|M|H> 0x<код,4> <src,4> <val,11> <описание>\r\n".
 *         RTC - "ГГГГ-ММ-ДД ЧЧ:ММ:СС" из Unix-времени, при rtc_time == 0 - 19 пробелов.
 * @param  o           приёмник
 * @param  code        код лога
 * @param  source_id   идентификатор источника события
 * @param  priority    приоритет кода
 * @param  value       значение переменной
 * @param  systick     HAL_GetTick() на момент события
 * @param  rtc_time    Unix-время RTC на момент события, либо 0
 * @param  description текстовое описание кода, либо NULL */
static void logger_format_line(logger_out_t *o, uint16_t code, uint16_t source_id,
                                LOGGER_Priority_t priority, int32_t value, uint32_t systick,
                                uint32_t rtc_time, const char *description)
{
    char rtc_text[LOGGER_RTC_TEXT_WIDTH + 1U];
    LOGGER_UnixTimeToText(rtc_time, rtc_text);
    logger_out_puts(o, rtc_text);
    logger_out_putc(o, ' ');
    logger_out_uint32(o, systick, 8U);
    logger_out_putc(o, ' ');
    logger_out_puts(o, (code <= LOGGER_INTERNAL_CODE_MAX) ? "LOGGER  " : LOGGER_GetGroupName(code));
    logger_out_putc(o, ' ');
    logger_out_putc(o, logger_priority_letter(priority));
    logger_out_puts(o, " 0x");
    logger_out_hex16(o, code);
    logger_out_putc(o, ' ');
    logger_out_hex16(o, source_id);
    logger_out_putc(o, ' ');
    logger_out_int32(o, value, 11U);
    logger_out_putc(o, ' ');
    logger_out_puts(o, (description != NULL) ? description : "???");
    logger_out_puts(o, "\r\n");
}
#ifndef LOGGER_NO_ITM
/** @brief Вывод по умолчанию в SWO - используется, если console_fn не задана.
 * @param  code        код лога
 * @param  source_id   идентификатор источника события
 * @param  priority    приоритет кода
 * @param  value       значение переменной
 * @param  systick     HAL_GetTick() на момент события
 * @param  rtc_time    показание RTC на момент события, либо 0
 * @param  description текстовое описание кода, либо NULL */
static void logger_swo_output(uint16_t code, uint16_t source_id, LOGGER_Priority_t priority,
                               int32_t value, uint32_t systick, uint32_t rtc_time,
                               const char *description)
{
    logger_out_t out = { NULL, 0U, 0U, 0U };
    logger_format_line(&out, code, source_id, priority, value, systick, rtc_time, description);
}
#endif /* !LOGGER_NO_ITM */

/** @brief Формирует строку лога в буфер - см. полное описание в logger.h.
 * @param  buf         буфер результата (может быть NULL при size == 0)
 * @param  size        размер buf в байтах
 * @param  code        код лога
 * @param  source_id   идентификатор источника события
 * @param  priority    приоритет кода
 * @param  value       значение переменной
 * @param  systick     HAL_GetTick() на момент события
 * @param  rtc_time    показание RTC на момент события, либо 0
 * @param  description текстовое описание кода, либо NULL
 * @return длина полной строки без '\0' (как у snprintf) */
size_t LOGGER_FormatConsoleLine(char *buf, size_t size, uint16_t code, uint16_t source_id,
                                 LOGGER_Priority_t priority, int32_t value, uint32_t systick,
                                 uint32_t rtc_time, const char *description)
{
    logger_out_t out = { buf, size, 0U, 1U };

    logger_format_line(&out, code, source_id, priority, value, systick, rtc_time, description);

    if ((buf != NULL) && (size > 0U))
    {
        buf[(out.len < size) ? out.len : (size - 1U)] = '\0';
    }
    return out.len;
}

/* ------------------------------------------------------------------------ */
/*  Поиск записи в таблице кодов (двоичный поиск / служебная таблица)      */
/* ------------------------------------------------------------------------ */

/** @brief Линейный поиск кода в служебной таблице LOGGER_InternalTable
 *         (0x0000-0x00FF).
 * @param  code код лога
 * @return указатель на запись; NULL, если код не найден */
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

/** @brief Двоичный поиск кода в пользовательской таблице (logger_codes.h).
 *         Возвращает индекс (не указатель), чтобы сразу обновить статистику
 *         в s_code_stats. code гарантированно > LOGGER_INTERNAL_CODE_MAX.
 * @param  code код лога
 * @return индекс в LOGGER_LogTable; LOGGER_LOG_TABLE_SIZE, если не найден */
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
/*  Короткая критическая секция (PRIMASK) - защищает s_buffer/s_buffer_count */
/*  и s_max_priority(_count) от гонки при конкурентных вызовах              */
/*  LOGGER_Log()/LOGGER_Mark() из ISR и main без синхронизации на стороне   */
/*  вызывающего (см. README, "Честные ограничения"). Save/restore PRIMASK,  */
/*  а не безусловный __enable_irq() - корректно и при вложенном вызове из   */
/*  уже замаскированного контекста. Сам write_fn (может быть медленным -    */
/*  запись в flash и т.п.) ВСЕГДА вызывается вне критической секции - см.   */
/*  logger_flush_raw().                                                     */
/* ------------------------------------------------------------------------ */

/** @brief Входит в критическую секцию - сохраняет PRIMASK и отключает прерывания.
 * @return предыдущее значение PRIMASK, передать в logger_critical_exit() */
static uint32_t logger_critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/** @brief Выходит из критической секции, восстанавливая PRIMASK.
 * @param  primask значение, полученное от logger_critical_enter() */
static void logger_critical_exit(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/* ------------------------------------------------------------------------ */
/*  Буферизация                                                             */
/* ------------------------------------------------------------------------ */

/** @brief Общий конвейер вывода + (опционально) буферизации и статистики -
 *         см. полный комментарий перед определением ниже.
 * @param  code      код лога
 * @param  source_id идентификатор источника события
 * @param  value     значение переменной
 * @param  systick   уже захваченный HAL_GetTick()
 * @param  persist   1 - можно буферизовать (обычные логи); 0 - только вывод
 *                    (служебные логи о самой библиотеке) */
static void logger_emit(uint16_t code, uint16_t source_id, int32_t value, uint32_t systick,
                         uint8_t persist);

/** @brief Сообщает об ошибке самой библиотеки служебной записью. Защита от
 *         лавины и зацикливания: (1) запись выводится только на 1-м, 2-м, 4-м,
 *         8-м... срабатывании данного вида (степени двойки), value = номер
 *         срабатывания - по разнице соседних записей видно, сколько подавлено;
 *         точный счёт - LOGGER_GetErrorCount(); (2) пока идёт вывод другой
 *         такой записи (s_reporting), новая не порождается - ошибка только
 *         считается: служебная запись не может вызвать ещё одну служебную.
 * @param  kind      вид ошибки
 * @param  source_id пояснение к ошибке (смысл зависит от кода)
 * @param  persist   1 - можно сохранить в память; 0 - только вывод (кольцо
 *                    полно/сброшено либо идёт аварийное сохранение) */
static void logger_report_error(logger_err_kind_t kind, uint16_t source_id, uint8_t persist)
{
    uint32_t primask = logger_critical_enter();

    if (s_err_count[kind] != UINT32_MAX)
    {
        s_err_count[kind]++;
    }
    uint32_t n    = s_err_count[kind];
    uint8_t  emit = ((((n & (n - 1U)) == 0U)) && (s_reporting == 0U)) ? 1U : 0U;
    if (emit != 0U)
    {
        s_reporting = 1U;
    }
    logger_critical_exit(primask);

    if (emit == 0U)
    {
        return;
    }

    logger_emit(s_err_codes[kind], source_id, (int32_t)n, HAL_GetTick(), persist);
    s_reporting = 0U;
}

/** @brief Кладёт запись в кольцо. Вызывать ТОЛЬКО под критической секцией и
 *         когда в кольце есть свободное место. Обновляет пик заполнения и
 *         счётчик триггера сброса по приоритету. */
static void logger_ring_put_locked(uint16_t code, LOGGER_Priority_t priority, uint16_t source_id,
                                    int32_t value, uint32_t systick, uint32_t rtc_time)
{
    uint16_t index = (uint16_t)(s_buffer_head + s_buffer_count);
    if (index >= (uint16_t)LOGGER_BUFFER_CAPACITY)
    {
        index = (uint16_t)(index - (uint16_t)LOGGER_BUFFER_CAPACITY);
    }

    s_buffer[index].code      = code;
    s_buffer[index].source_id = source_id;
    s_buffer[index].value     = value;
    s_buffer[index].systick   = systick;
    s_buffer[index].rtc_time  = rtc_time;
    s_buffer_count            = (uint16_t)(s_buffer_count + 1U);

    if (s_buffer_count > s_high_water)
    {
        s_high_water = s_buffer_count;
    }

    if ((s_max_priority_count == 0U) || (priority > s_max_priority))
    {
        s_max_priority       = priority;
        s_max_priority_count = 1U;
    }
    else if (priority == s_max_priority)
    {
        s_max_priority_count++;
    }
    /* priority < s_max_priority: запись не учитывается в счётчике
     * триггера сброса - см. архитектуру в logger.h */
}

/** @brief Если после потерь появилось место - кладёт в кольцо сводку
 *         LOGGER_INTERNAL_CODE_DROPPED_SUMMARY (value = потеряно с прошлой
 *         сводки), чтобы потери были видны и в сохранённом логе, а не только
 *         в консоли. Под критической секцией; место проверяет вызывающий
 *         (нужно свободное место под сводку и под свою запись).
 * @param  systick  время сводки
 * @param  rtc_time RTC сводки либо 0 */
static void logger_summary_put_locked(uint32_t systick, uint32_t rtc_time)
{
    uint32_t lost = s_dropped_unreported;

    s_dropped_unreported = 0U;
    logger_ring_put_locked(LOGGER_INTERNAL_CODE_DROPPED_SUMMARY, LOGGER_PRIORITY_HIGH, 0U,
                            (lost > (uint32_t)INT32_MAX) ? INT32_MAX : (int32_t)lost, systick,
                            rtc_time);
}

/** @brief Отправляет в write_fn один непрерывный сегмент кольца (от головы
 *         до конца массива либо до последней записи - что раньше) и
 *         освобождает его. Записи, добавленные конкурентно ПОКА идёт write_fn
 *         (I/O вне критической секции, может занять заметное время),
 *         ложатся в свободную часть кольца и не мешаются - ничего не
 *         двигается и не теряется. Эксклюзивность между конкурентными
 *         сбросами - s_flush_in_progress: если сброс уже идёт в другом
 *         контексте, вызов сразу возвращает 0 (иначе два сброса освободили бы
 *         одни и те же записи дважды - underflow, найден тестом на v1.8).
 *         Без служебного лога (нужно для LOGGER_EmergencySave()). Вызывается
 *         только при s_buffering_active != 0.
 * @return число отправленных в write_fn записей; 0, если кольцо пусто либо
 *          сброс уже шёл в другом контексте */
static uint16_t logger_flush_raw(void)
{
    uint32_t primask = logger_critical_enter();

    if ((s_buffer_count > (uint16_t)LOGGER_BUFFER_CAPACITY) ||
        (s_buffer_head >= (uint16_t)LOGGER_BUFFER_CAPACITY))
    {
        /* Самовосстановление при повреждённых индексах - не читаем write_fn
         * за границей s_buffer; накопленное отбрасывается и учитывается. */
        uint16_t lost = (s_buffer_count > (uint16_t)LOGGER_BUFFER_CAPACITY)
                            ? (uint16_t)LOGGER_BUFFER_CAPACITY : s_buffer_count;
        s_dropped_count     += lost;
        s_dropped_unreported += lost;
        s_buffer_head        = 0U;
        s_buffer_count       = 0U;
        s_max_priority_count = 0U;
        logger_critical_exit(primask);
        logger_report_error(LOGGER_ERR_RING_CORRUPT, lost, 0U);
        return 0U;
    }

    if ((s_flush_in_progress != 0U) || (s_buffer_count == 0U))
    {
        logger_critical_exit(primask);
        return 0U;
    }

    uint16_t to_end  = (uint16_t)((uint16_t)LOGGER_BUFFER_CAPACITY - s_buffer_head);
    uint16_t segment = (s_buffer_count < to_end) ? s_buffer_count : to_end;
    uint16_t head    = s_buffer_head;

    s_flush_in_progress = 1U;
    logger_critical_exit(primask);

    s_config.write_fn(s_config.write_context, (const uint8_t *)&s_buffer[head],
                       (uint32_t)segment * (uint32_t)sizeof(LOGGER_Record_t));

    primask = logger_critical_enter();
    s_buffer_head = (uint16_t)(head + segment);
    if (s_buffer_head >= (uint16_t)LOGGER_BUFFER_CAPACITY)
    {
        s_buffer_head = 0U;
    }
    s_buffer_count = (s_buffer_count >= segment) ? (uint16_t)(s_buffer_count - segment) : 0U;
    /* Приоритет оставшегося хвоста не пересчитывается - следующий порог по
     * нему наберётся заново (см. README): лишь чуть отложит сброс по порогу,
     * данные не теряет (а сброс по уровню заполнения от этого не зависит). */
    s_max_priority_count = 0U;
    s_flush_in_progress  = 0U;
    logger_critical_exit(primask);

    return segment;
}

/** @brief Сбрасывает всё накопленное (до 2 сегментов - кольцо могло
 *         "перевалить" через конец массива) без служебного лога.
 * @return суммарное число отправленных записей */
static uint16_t logger_flush_all_raw(void)
{
    uint16_t total = 0U;

    for (uint8_t pass = 0U; pass < 2U; pass++)
    {
        uint16_t n = logger_flush_raw();
        if (n == 0U)
        {
            break;
        }
        total = (uint16_t)(total + n);
    }
    return total;
}

/** @brief Сбрасывает буфер в write_fn (если не пуст) и логирует служебное
 *         событие LOGGER_INTERNAL_CODE_FLUSH только выводом (persist=0),
 *         чтобы не порождать рекурсивный сброс буфера о самом себе. */
static void logger_flush_internal(void)
{
    if (s_dropped_unreported != 0U)
    {
        /* Сводку о потерях кладём перед сбросом - уйдёт в память вместе с ним. */
        uint32_t systick = HAL_GetTick();
        uint32_t rtc     = (s_config.rtc_time_fn != NULL)
                               ? s_config.rtc_time_fn(s_config.rtc_context) : 0U;
        uint32_t primask = logger_critical_enter();
        if ((s_dropped_unreported != 0U) && (s_buffer_count < (uint16_t)LOGGER_BUFFER_CAPACITY))
        {
            logger_summary_put_locked(systick, rtc);
        }
        logger_critical_exit(primask);
    }

    if (s_buffer_count == 0U)
    {
        return;
    }

    uint16_t flushed_count = logger_flush_all_raw();
    if (flushed_count == 0U)
    {
        return;
    }

    logger_emit(LOGGER_INTERNAL_CODE_FLUSH, 0U, (int32_t)flushed_count, HAL_GetTick(), 0U);
}

/** @brief Добавляет запись в кольцо и инициирует сброс по порогу приоритета
 *         ЛИБО по уровню заполнения LOGGER_BUFFER_WATERMARK (чтобы к моменту
 *         полного кольца write_fn уже работал, а приём продолжался в
 *         свободной части). Если кольцо всё же полно: пробует освободить
 *         место сбросом; если сброс уже идёт в другом контексте - в потоке
 *         (не ISR, прерывания разрешены) ждёт до LOGGER_OVERFLOW_WAIT_MS, в
 *         ISR/при замаскированных прерываниях не ждёт. Только если освободить
 *         место не вышло - запись отбрасывается и считается
 *         (LOGGER_GetDroppedCount()). Резервирование - под короткой
 *         критической секцией, I/O и ожидание - вне её.
 * @param  code      код лога
 * @param  priority  приоритет кода
 * @param  source_id идентификатор источника события
 * @param  value     значение переменной
 * @param  systick   HAL_GetTick() на момент события
 * @param  rtc_time  показание RTC на момент события, либо 0 */
static void logger_buffer_push(uint16_t code, LOGGER_Priority_t priority, uint16_t source_id,
                                int32_t value, uint32_t systick, uint32_t rtc_time)
{
    uint32_t wait_start = 0U;
    uint8_t  waiting    = 0U;
    uint8_t  need_flush = 0U;

    for (;;)
    {
        uint32_t primask = logger_critical_enter();

        if (s_buffer_count < (uint16_t)LOGGER_BUFFER_CAPACITY)
        {
            /* Были потери, а теперь есть место под сводку И под запись -
             * сначала сводка (value = сколько потеряно), потом сама запись. */
            if ((s_dropped_unreported != 0U) &&
                ((uint16_t)(s_buffer_count + 1U) < (uint16_t)LOGGER_BUFFER_CAPACITY))
            {
                logger_summary_put_locked(systick, rtc_time);
            }

            logger_ring_put_locked(code, priority, source_id, value, systick, rtc_time);

            need_flush = ((s_max_priority_count >= s_threshold[s_max_priority]) ||
                          (s_buffer_count >= (uint16_t)LOGGER_BUFFER_WATERMARK)) ? 1U : 0U;

            logger_critical_exit(primask);
            break;
        }

        logger_critical_exit(primask);

        /* Кольцо полно. Пробуем освободить место сбросом. */
        if (logger_flush_raw() != 0U)
        {
            continue;
        }

        /* Сброс уже идёт в другом контексте (или write_fn не вернулся).
         * Ждать можно только в потоке с разрешёнными прерываниями - в ISR
         * либо при PRIMASK=1 HAL_GetTick() не растёт, ожидание зависло бы. */
        uint8_t can_wait = ((LOGGER_OVERFLOW_WAIT_MS != 0U) &&
                            (__get_IPSR() == 0U) && ((primask & 1U) == 0U)) ? 1U : 0U;
        if (can_wait != 0U)
        {
            if (waiting == 0U)
            {
                waiting    = 1U;
                wait_start = HAL_GetTick();
                continue;
            }
            if ((HAL_GetTick() - wait_start) < (uint32_t)LOGGER_OVERFLOW_WAIT_MS)
            {
                continue;
            }
        }

        /* Освободить место не удалось - честно считаем потерю. */
        primask = logger_critical_enter();
        s_dropped_count++;
        if (s_dropped_unreported != UINT32_MAX)
        {
            s_dropped_unreported++;
        }
        logger_critical_exit(primask);
        if (waiting != 0U)
        {
            logger_report_error(LOGGER_ERR_WAIT_TIMEOUT, code, 0U);
        }
        logger_report_error(LOGGER_ERR_OVERFLOW, code, 0U);
        return;
    }

    if (need_flush != 0U)
    {
        logger_flush_internal();
    }
}

/* ------------------------------------------------------------------------ */
/*  Общая статистика частоты вызовов (меток и обычных кодов)                */
/* ------------------------------------------------------------------------ */

/** @brief Обновляет call_count/first_systick/last_systick одной записи
 *         статистики - общая логика для s_mark_stats и s_code_stats.
 * @param  stats запись статистики для обновления
 * @param  now   текущее время (систика/DWT, в зависимости от вызывающей стороны) */
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

/** @brief Общая реализация для LOGGER_Log()/LOGGER_Mark()/служебных логов.
 *         persist=0 используется только служебными логами о самой библиотеке
 *         (INIT/FLUSH), которые не должны попадать в буфер.
 * @param  code      код лога
 * @param  source_id идентификатор источника события
 * @param  value     значение переменной
 * @param  systick   уже захваченный HAL_GetTick()
 * @param  persist   1 - можно буферизовать; 0 - только вывод */
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

    if (entry == NULL)
    {
        /* Код, которого нет в таблице: сама запись не сохраняется (нет приоритета
         * и описания) - сообщаем об этом отдельной служебной записью. */
        logger_report_error(LOGGER_ERR_UNKNOWN_CODE, code, 1U);
    }
}

/* ------------------------------------------------------------------------ */
/*  Инициализация                                                           */
/* ------------------------------------------------------------------------ */

/** @brief Сообщает причину отказа LOGGER_Init() служебным логом
 *         LOGGER_INTERNAL_CODE_INIT_FAIL - только вывод (console_fn из
 *         переданного config, либо SWO; не буферизуется, s_config ещё не
 *         применён).
 * @param  config     конфигурация, переданная в LOGGER_Init() (может быть NULL)
 * @param  reason     LOGGER_INIT_FAIL_* - причина отказа (идёт в value)
 * @param  entry_code код проблемной записи таблицы, либо 0 (идёт в source_id)
 * @return всегда HAL_ERROR - чтобы писать "return logger_init_fail(...)" */
static HAL_StatusTypeDef logger_init_fail(const LOGGER_Config_t *config, int32_t reason,
                                           uint16_t entry_code)
{
    const LOGGER_LogEntry_t *entry   = logger_find_internal_entry(LOGGER_INTERNAL_CODE_INIT_FAIL);
    const char              *desc    = (entry != NULL) ? entry->description : NULL;
    uint32_t                 systick = HAL_GetTick();

    if ((config != NULL) && (config->console_fn != NULL))
    {
        config->console_fn(config->console_context, LOGGER_INTERNAL_CODE_INIT_FAIL, entry_code,
                            LOGGER_PRIORITY_HIGH, reason, systick, 0U, desc);
    }
#ifndef LOGGER_NO_ITM
    else
    {
        logger_swo_output(LOGGER_INTERNAL_CODE_INIT_FAIL, entry_code, LOGGER_PRIORITY_HIGH,
                           reason, systick, 0U, desc);
    }
#endif
    return HAL_ERROR;
}

/** @brief Инициализирует библиотеку - см. полное описание в logger.h.
 *         При отказе дополнительно выводит причину (LOGGER_INIT_FAIL_*) и код
 *         проблемной записи таблицы служебным логом INIT_FAIL.
 * @param  config конфигурация логгера
 * @return HAL_OK при успехе; HAL_ERROR при некорректной таблице кодов/config */
HAL_StatusTypeDef LOGGER_Init(const LOGGER_Config_t *config)
{
    if (config == NULL)
    {
        return logger_init_fail(NULL, LOGGER_INIT_FAIL_CONFIG_NULL, 0U);
    }

    /* Проверка таблицы кодов - один раз, здесь, чтобы LOGGER_Log() мог
     * доверять её корректности и использовать быстрый двоичный поиск без
     * дополнительных проверок в рантайме. */
    if (LOGGER_LOG_TABLE_SIZE == 0U)
    {
        return logger_init_fail(config, LOGGER_INIT_FAIL_TABLE_EMPTY, 0U);
    }

    for (uint32_t i = 0U; i < LOGGER_LOG_TABLE_SIZE; i++)
    {
        const LOGGER_LogEntry_t *entry = &LOGGER_LogTable[i];

        if (entry->code <= LOGGER_INTERNAL_CODE_MAX)
        {
            /* код из зарезервированного служебного диапазона 0x0000-0x00FF */
            return logger_init_fail(config, LOGGER_INIT_FAIL_CODE_RESERVED, entry->code);
        }

        if ((entry->priority != LOGGER_PRIORITY_LOW) &&
            (entry->priority != LOGGER_PRIORITY_MEDIUM) &&
            (entry->priority != LOGGER_PRIORITY_HIGH))
        {
            return logger_init_fail(config, LOGGER_INIT_FAIL_BAD_PRIORITY, entry->code);
        }

        if (entry->description == NULL)
        {
            return logger_init_fail(config, LOGGER_INIT_FAIL_DESC_NULL, entry->code);
        }

        uint32_t len = 0U;
        while ((len <= LOGGER_MAX_DESCRIPTION_LENGTH) && (entry->description[len] != '\0'))
        {
            len++;
        }
        if (len > LOGGER_MAX_DESCRIPTION_LENGTH)
        {
            /* описание длиннее LOGGER_MAX_DESCRIPTION_LENGTH */
            return logger_init_fail(config, LOGGER_INIT_FAIL_DESC_TOO_LONG, entry->code);
        }

        if ((i > 0U) && (entry->code <= LOGGER_LogTable[i - 1U].code))
        {
            /* таблица не отсортирована по возрастанию либо есть повтор кода */
            return logger_init_fail(config, LOGGER_INIT_FAIL_NOT_SORTED, entry->code);
        }
    }

    if (config->write_fn != NULL)
    {
        if ((config->threshold_low == 0U)    || (config->threshold_low    > LOGGER_BUFFER_CAPACITY) ||
            (config->threshold_medium == 0U) || (config->threshold_medium > LOGGER_BUFFER_CAPACITY) ||
            (config->threshold_high == 0U)   || (config->threshold_high   > LOGGER_BUFFER_CAPACITY))
        {
            return logger_init_fail(config, LOGGER_INIT_FAIL_BAD_THRESHOLD, 0U);
        }
    }

#ifdef LOGGER_NO_ITM
    /* На ядрах без ITM (LOGGER_NO_ITM определён - см. README.md) вывода по
     * умолчанию не существует вовсе - console_fn обязательна, иначе логи
     * будут просто молча теряться. */
    if (config->console_fn == NULL)
    {
        return logger_init_fail(config, LOGGER_INIT_FAIL_CONSOLE_REQUIRED, 0U);
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
    s_buffer_head         = 0U;
    s_buffer_count        = 0U;
    s_high_water          = 0U;
    s_max_priority_count  = 0U;
    s_max_priority        = LOGGER_PRIORITY_LOW;

    s_initialized         = 1U;

    logger_emit(LOGGER_INTERNAL_CODE_INIT, 0U, (int32_t)LOGGER_LOG_TABLE_SIZE, HAL_GetTick(), 0U);

    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Приём лога - главная горячая функция                                    */
/* ------------------------------------------------------------------------ */

/** @brief Записывает лог - см. полное описание в logger.h.
 * @param  code      код лога
 * @param  source_id идентификатор источника события
 * @param  value     значение переменной */
void LOGGER_Log(uint16_t code, uint16_t source_id, int32_t value)
{
    if (s_initialized == 0U)
    {
        logger_report_error(LOGGER_ERR_NOT_INIT, LOGGER_NOT_INIT_API_LOG, 0U);
    }
    logger_emit(code, source_id, value, HAL_GetTick(), 1U);
}

/* ------------------------------------------------------------------------ */
/*  Временные метки и их частота                                            */
/* ------------------------------------------------------------------------ */

/** @brief Регистрирует временную метку - см. полное описание в logger.h.
 * @param  mark_id идентификатор метки */
void LOGGER_Mark(uint16_t mark_id)
{
    uint32_t now = HAL_GetTick(); /* попадает в саму запись лога/буфер - всегда мс */

    if (s_initialized == 0U)
    {
        logger_report_error(LOGGER_ERR_NOT_INIT, LOGGER_NOT_INIT_API_MARK, 0U);
    }

    if (mark_id < LOGGER_MARK_MAX_IDS)
    {
        /* Статистика частоты метки - в единицах LOGGER_MARK_TIME_SOURCE
         * (мс через SysTick по умолчанию, либо мс/мкс через DWT), НЕ путать
         * с 'now' выше, который идёт в саму запись лога. */
        logger_freq_stats_bump(&s_mark_stats[mark_id], logger_mark_now());
    }
    else
    {
        logger_report_error(LOGGER_ERR_MARK_RANGE, mark_id, 1U);
    }

    logger_emit(LOGGER_INTERNAL_CODE_MARK, mark_id, (int32_t)mark_id, now, 1U);
}

/** @brief Возвращает частоту вызовов метки - см. полное описание в logger.h.
 * @param  mark_id       идентификатор метки
 * @param  out_frequency куда записать результат (Гц)
 * @return HAL_OK при успехе; HAL_ERROR при недостаточных данных/неверном mark_id */
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

/** @brief Возвращает частоту вызовов кода лога - см. полное описание в logger.h.
 * @param  code          код лога
 * @param  out_frequency куда записать результат (Гц)
 * @return HAL_OK при успехе; HAL_ERROR при недостаточных данных/коде не в таблице */
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

/** @brief Принудительно сбрасывает буфер в память - см. описание в logger.h.
 * @return HAL_OK при успехе; HAL_ERROR если буферизация не настроена */
HAL_StatusTypeDef LOGGER_Flush(void)
{
    if (!s_buffering_active)
    {
        logger_report_error(LOGGER_ERR_NOT_INIT, LOGGER_NOT_INIT_API_FLUSH, 0U);
        return HAL_ERROR;
    }
    logger_flush_internal();
    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Аварийное сохранение (сброс перед потерей питания/перезагрузкой)        */
/* ------------------------------------------------------------------------ */

/** @brief Аварийное сохранение буфера перед потерей питания - см. logger.h.
 * @return HAL_OK при успехе (в т.ч. если буфер пуст); HAL_ERROR если
 *          буферизация не настроена */
HAL_StatusTypeDef LOGGER_EmergencySave(void)
{
    if (!s_buffering_active)
    {
        logger_report_error(LOGGER_ERR_NOT_INIT, LOGGER_NOT_INIT_API_EMERGENCY_SAVE, 0U);
        return HAL_ERROR;
    }
    if (s_buffer_count == 0U)
    {
        return HAL_OK;
    }
    (void)logger_flush_all_raw();
    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Диагностика переполнения                                                */
/* ------------------------------------------------------------------------ */

/** @brief Сколько записей потеряно из-за переполнения - см. logger.h.
 * @return число потерянных записей с момента старта */
uint32_t LOGGER_GetDroppedCount(void)
{
    uint32_t primask = logger_critical_enter();
    uint32_t dropped = s_dropped_count;
    logger_critical_exit(primask);
    return dropped;
}

/** @brief Пик заполнения буфера - см. logger.h.
 * @return максимум одновременно лежавших в буфере записей с LOGGER_Init() */
uint16_t LOGGER_GetBufferHighWater(void)
{
    return s_high_water;
}

/** @brief Сколько раз сработала ошибка библиотеки - см. logger.h.
 * @param  code служебный код ошибки (LOGGER_INTERNAL_CODE_*)
 * @return число срабатываний с момента старта; 0 для кода, не являющегося
 *          ошибкой библиотеки */
uint32_t LOGGER_GetErrorCount(uint16_t code)
{
    uint32_t count = 0U;

    for (uint32_t i = 0U; i < (uint32_t)LOGGER_ERR_COUNT; i++)
    {
        if (s_err_codes[i] == code)
        {
            uint32_t primask = logger_critical_enter();
            count = s_err_count[i];
            logger_critical_exit(primask);
            break;
        }
    }
    return count;
}
