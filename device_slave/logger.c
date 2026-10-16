/**
 ******************************************************************************
 * @file    logger.c
 * @brief   Реализация библиотеки логирования, роль device_slave (см. logger.h).
 * @author  Mechanic
 * @date    13.10.2026
 * @version 2.0
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
 *         "<RTC,19> <мс,8> <узел,3> <ГРУППА,8> <L|M|H> 0x<код,4> <src,4> <val,11> <описание>+CRLF".
 *         RTC - "ГГГГ-ММ-ДД ЧЧ:ММ:СС" из Unix-времени, при rtc_time == 0 - 19 пробелов.
 * @param  o           приёмник
 * @param  node_id     0 - master, 1..255 - slave
 * @param  code        код лога
 * @param  source_id   идентификатор источника события
 * @param  priority    приоритет кода
 * @param  value       значение переменной
 * @param  systick     HAL_GetTick() на момент события
 * @param  rtc_time    Unix-время RTC на момент события, либо 0
 * @param  description текстовое описание кода, либо NULL */
static void logger_format_line(logger_out_t *o, uint8_t node_id, uint16_t code, uint16_t source_id,
                                LOGGER_Priority_t priority, int32_t value, uint32_t systick,
                                uint32_t rtc_time, const char *description)
{
    char rtc_text[LOGGER_RTC_TEXT_WIDTH + 1U];
    LOGGER_UnixTimeToText(rtc_time, rtc_text);
    logger_out_puts(o, rtc_text);
    logger_out_putc(o, ' ');
    logger_out_uint32(o, systick, 8U);
    logger_out_putc(o, ' ');
    logger_out_uint32(o, node_id, 3U);
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
 * @param  node_id     0 - master, 1..255 - slave
 * @param  code        код лога
 * @param  source_id   идентификатор источника события
 * @param  priority    приоритет кода
 * @param  value       значение переменной
 * @param  systick     HAL_GetTick() на момент события
 * @param  rtc_time    показание RTC на момент события, либо 0
 * @param  description текстовое описание кода, либо NULL */
static void logger_swo_output(uint8_t node_id, uint16_t code, uint16_t source_id, LOGGER_Priority_t priority,
                               int32_t value, uint32_t systick, uint32_t rtc_time,
                               const char *description)
{
    logger_out_t out = { NULL, 0U, 0U, 0U };
    logger_format_line(&out, node_id, code, source_id, priority, value, systick, rtc_time, description);
}
#endif /* !LOGGER_NO_ITM */

/** @brief Формирует строку лога в буфер - см. полное описание в logger.h.
 * @param  buf         буфер результата (может быть NULL при size == 0)
 * @param  size        размер buf в байтах
 * @param  node_id     0 - master, 1..255 - slave
 * @param  code        код лога
 * @param  source_id   идентификатор источника события
 * @param  priority    приоритет кода
 * @param  value       значение переменной
 * @param  systick     HAL_GetTick() на момент события
 * @param  rtc_time    показание RTC на момент события, либо 0
 * @param  description текстовое описание кода, либо NULL
 * @return длина полной строки без '\0' (как у snprintf) */
size_t LOGGER_FormatConsoleLine(char *buf, size_t size, uint8_t node_id, uint16_t code, uint16_t source_id,
                                 LOGGER_Priority_t priority, int32_t value, uint32_t systick,
                                 uint32_t rtc_time, const char *description)
{
    logger_out_t out = { buf, size, 0U, 1U };

    logger_format_line(&out, node_id, code, source_id, priority, value, systick, rtc_time, description);

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
/*  Состояние модуля (статика, без malloc)                                  */
/* ------------------------------------------------------------------------ */

/* Статика обнуляется при старте (BSS): до LOGGER_Init() s_config.send_fn и
 * console_fn равны NULL - LOGGER_Log() работает в режиме "только вывод в SWO". */

/** Один кадр в очереди на отправку: seq присваивается в момент записи, в том
 *  числе потерянным кадрам (номер "съедается") - поэтому master видит потерю
 *  по разрыву номеров. */
typedef struct
{
    uint16_t code;
    uint16_t source_id;
    int32_t  value;
    uint8_t  seq;
} logger_slot_t;

static LOGGER_Config_t s_config;
static logger_slot_t   s_ring[LOGGER_BUFFER_CAPACITY];
static uint16_t        s_head;
static uint16_t        s_count;
static uint8_t         s_seq;
static uint8_t         s_sending;
static uint32_t        s_dropped_count;
static uint32_t        s_dropped_unreported;

/* ------------------------------------------------------------------------ */
/*  Очередь                                                                 */
/* ------------------------------------------------------------------------ */

/** @brief Кладёт кадр в хвост очереди. Только под критической секцией и при
 *         наличии свободного места.
 * @param  code      код лога
 * @param  source_id идентификатор источника
 * @param  value     значение */
static void logger_enqueue_locked(uint16_t code, uint16_t source_id, int32_t value)
{
    uint16_t index = (uint16_t)(s_head + s_count);
    if (index >= (uint16_t)LOGGER_BUFFER_CAPACITY)
    {
        index = (uint16_t)(index - (uint16_t)LOGGER_BUFFER_CAPACITY);
    }
    s_ring[index].code      = code;
    s_ring[index].source_id = source_id;
    s_ring[index].value     = value;
    s_ring[index].seq       = s_seq;
    s_seq                   = (uint8_t)(s_seq + 1U);
    s_count                 = (uint16_t)(s_count + 1U);
}

/** @brief Ставит запись в очередь на отправку; при полной очереди теряет её,
 *         сохранив номер seq (разрыв увидит master) и посчитав потерю.
 * @param  code      код лога
 * @param  source_id идентификатор источника
 * @param  value     значение */
static void logger_queue_put(uint16_t code, uint16_t source_id, int32_t value)
{
    uint32_t primask = logger_critical_enter();

    if (s_count < (uint16_t)LOGGER_BUFFER_CAPACITY)
    {
        /* Были потери, а теперь есть место и под сводку, и под запись. */
        if ((s_dropped_unreported != 0U) &&
            ((uint16_t)(s_count + 1U) < (uint16_t)LOGGER_BUFFER_CAPACITY))
        {
            uint32_t lost = s_dropped_unreported;
            s_dropped_unreported = 0U;
            logger_enqueue_locked(LOGGER_INTERNAL_CODE_DROPPED_SUMMARY, 0U,
                                   (lost > (uint32_t)INT32_MAX) ? INT32_MAX : (int32_t)lost);
        }
        logger_enqueue_locked(code, source_id, value);
    }
    else
    {
        s_seq = (uint8_t)(s_seq + 1U);
        s_dropped_count++;
        if (s_dropped_unreported != UINT32_MAX)
        {
            s_dropped_unreported++;
        }
    }

    logger_critical_exit(primask);
}

/* ------------------------------------------------------------------------ */
/*  Вывод                                                                   */
/* ------------------------------------------------------------------------ */

/** @brief Выводит запись в консоль/SWO.
 * @param  code        код лога
 * @param  source_id   идентификатор источника
 * @param  priority    приоритет кода
 * @param  value       значение
 * @param  description описание из таблицы либо NULL */
static void logger_output(uint16_t code, uint16_t source_id, LOGGER_Priority_t priority,
                           int32_t value, const char *description)
{
    uint32_t systick = HAL_GetTick();

    if (s_config.console_fn != NULL)
    {
        s_config.console_fn(s_config.console_context, s_config.node_id, code, source_id, priority,
                             value, systick, 0U, description);
    }
#ifndef LOGGER_NO_ITM
    else
    {
        logger_swo_output(s_config.node_id, code, source_id, priority, value, systick, 0U, description);
    }
#endif
}

/* ------------------------------------------------------------------------ */
/*  Публичный API                                                           */
/* ------------------------------------------------------------------------ */

/** @brief Инициализация slave - см. logger.h.
 * @param  config конфигурация
 * @retval HAL_OK / HAL_ERROR */
HAL_StatusTypeDef LOGGER_Init(const LOGGER_Config_t *config)
{
    if ((config == NULL) || (config->node_id == 0U) ||
        ((uint32_t)config->send_min_priority >= LOGGER_PRIORITY_COUNT))
    {
        return HAL_ERROR;
    }
#ifdef LOGGER_NO_ITM
    if (config->console_fn == NULL)
    {
        return HAL_ERROR;
    }
#endif

    uint32_t primask = logger_critical_enter();
    s_config             = *config;
    s_head               = 0U;
    s_count              = 0U;
    s_seq                = 0U;
    s_sending            = 0U;
    s_dropped_count      = 0U;
    s_dropped_unreported = 0U;
    logger_critical_exit(primask);

    logger_output(LOGGER_INTERNAL_CODE_INIT, 0U, LOGGER_PRIORITY_LOW, (int32_t)LOGGER_LOG_TABLE_SIZE,
                   "инициализация выполнена");
    return HAL_OK;
}

/** @brief Логирование события - см. logger.h.
 * @param  code      код лога
 * @param  source_id идентификатор источника
 * @param  value     значение */
void LOGGER_Log(uint16_t code, uint16_t source_id, int32_t value)
{
    const LOGGER_LogEntry_t *entry = NULL;

    if (code <= LOGGER_INTERNAL_CODE_MAX)
    {
        entry = logger_find_internal_entry(code);
    }
    else
    {
        uint32_t index = logger_find_user_entry_index(code);
        if (index < LOGGER_LOG_TABLE_SIZE)
        {
            entry = &LOGGER_LogTable[index];
        }
    }

    LOGGER_Priority_t priority = (entry != NULL) ? entry->priority : LOGGER_PRIORITY_HIGH;

    logger_output(code, source_id, priority, value, (entry != NULL) ? entry->description : NULL);

    if ((s_config.send_fn != NULL) && (priority >= s_config.send_min_priority))
    {
        logger_queue_put(code, source_id, value);
    }
}

/** @brief Отправка накопленных кадров master - см. logger.h.
 * @return сколько кадров отправлено */
uint8_t LOGGER_Process(void)
{
    uint8_t sent = 0U;

    if (s_config.send_fn == NULL)
    {
        return 0U;
    }

    while (sent < (uint8_t)LOGGER_MAX_FRAMES_PER_PROCESS)
    {
        uint32_t primask = logger_critical_enter();
        if ((s_sending != 0U) || (s_count == 0U))
        {
            logger_critical_exit(primask);
            break;
        }
        s_sending = 1U;
        logger_slot_t slot = s_ring[s_head];
        logger_critical_exit(primask);

        LOGGER_Frame_t frame;
        frame.node_id   = s_config.node_id;
        frame.seq       = slot.seq;
        frame.code      = slot.code;
        frame.source_id = slot.source_id;
        frame.value     = slot.value;

        HAL_StatusTypeDef status = s_config.send_fn(s_config.send_context, &frame);

        primask = logger_critical_enter();
        s_sending = 0U;
        if (status == HAL_OK)
        {
            s_head = (uint16_t)(s_head + 1U);
            if (s_head >= (uint16_t)LOGGER_BUFFER_CAPACITY)
            {
                s_head = 0U;
            }
            s_count = (uint16_t)(s_count - 1U);
            sent++;
        }
        logger_critical_exit(primask);

        if (status != HAL_OK)
        {
            break;
        }
    }
    return sent;
}

/** @brief Потеряно кадров локально - см. logger.h.
 * @return число потерь */
uint32_t LOGGER_GetDroppedCount(void)
{
    uint32_t primask = logger_critical_enter();
    uint32_t value   = s_dropped_count;
    logger_critical_exit(primask);
    return value;
}

/** @brief Кадров в очереди - см. logger.h.
 * @return число кадров */
uint16_t LOGGER_GetPending(void)
{
    uint32_t primask = logger_critical_enter();
    uint16_t value   = s_count;
    logger_critical_exit(primask);
    return value;
}
