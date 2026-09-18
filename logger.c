/**
 ******************************************************************************
 * @file    logger.c
 * @brief   Реализация библиотеки логирования (см. logger.h).
 * @author  Mechanic
 * @date    18.09.2026
 * @version 1.1
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

/** Печатает 32-битное число в HEX, ровно 8 символов, без ведущего "0x". */
static void logger_swo_put_hex32(uint32_t value)
{
    static const char hex_digits[] = "0123456789ABCDEF"; /* 17 байт с '\0', не используется как строка */
    char buf[8];

    for (int8_t i = 7; i >= 0; i--)
    {
        buf[i] = hex_digits[value & 0xFU];
        value >>= 4;
    }
    for (uint8_t i = 0U; i < 8U; i++)
    {
        logger_swo_putc(buf[i]);
    }
}

/** Печатает знаковое 32-битное число в десятичном виде, без ведущих нулей. */
static void logger_swo_put_int32(int32_t value)
{
    char buf[10]; /* максимум 10 цифр у 2147483648 */
    uint8_t idx = 0U;
    uint32_t u;

    if (value < 0)
    {
        logger_swo_putc('-');
        /* аккуратно с INT32_MIN - его положительной пары не существует в int32_t */
        u = (uint32_t)(-(value + 1)) + 1U;
    }
    else
    {
        u = (uint32_t)value;
    }

    if (u == 0U)
    {
        buf[idx] = '0';
        idx++;
    }
    else
    {
        while (u != 0U)
        {
            buf[idx] = (char)('0' + (u % 10U));
            idx++;
            u /= 10U;
        }
    }

    while (idx > 0U)
    {
        idx--;
        logger_swo_putc(buf[idx]);
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
static void logger_swo_output(uint32_t code, LOGGER_Priority_t priority, int32_t value,
                               const char *description)
{
    logger_swo_puts("[LOG] 0x");
    logger_swo_put_hex32(code);
    logger_swo_puts(" [");
    logger_swo_puts(logger_priority_name(priority));
    logger_swo_puts("] val=");
    logger_swo_put_int32(value);
    logger_swo_puts(" : ");
    logger_swo_puts((description != NULL) ? description : "???");
    logger_swo_puts("\r\n");
}

/* ------------------------------------------------------------------------ */
/*  Поиск записи в таблице кодов (двоичный поиск)                          */
/* ------------------------------------------------------------------------ */

/** Таблица (logger_codes.h) гарантированно отсортирована по code по
 *  возрастанию, без повторов - это проверяет LOGGER_Init(). Поэтому здесь,
 *  в горячем пути, можно сразу использовать двоичный поиск без запасных
 *  проверок сортировки. */
static const LOGGER_LogEntry_t *logger_find_entry(uint32_t code)
{
    uint32_t lo = 0U;
    uint32_t hi = LOGGER_LOG_TABLE_SIZE;

    while (lo < hi)
    {
        uint32_t mid      = lo + ((hi - lo) >> 1U);
        uint32_t mid_code = LOGGER_LogTable[mid].code;

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

/** Сбрасывает накопленный буфер в write_fn (если он задан и буфер не пуст) и
 *  обнуляет счётчики. Вызывается только когда s_buffering_active != 0, то
 *  есть write_fn гарантированно не NULL. */
static void logger_flush_internal(void)
{
    if (s_buffer_count == 0U)
    {
        return;
    }

    s_config.write_fn(s_config.write_context, (const uint8_t *)s_buffer,
                       (uint32_t)s_buffer_count * (uint32_t)sizeof(LOGGER_Record_t));

    s_buffer_count       = 0U;
    s_max_priority_count = 0U;
}

/** Добавляет запись в буфер и, при достижении настроенного порога по текущему
 *  максимальному приоритету буфера, инициирует сброс в память. O(1). */
static void logger_buffer_push(uint32_t code, LOGGER_Priority_t priority, int32_t value)
{
    if (s_buffer_count >= (uint16_t)LOGGER_BUFFER_CAPACITY)
    {
        /* Физическая защита буфера - не должна срабатывать при корректно
         * настроенных порогах (см. LOGGER_Init), но не даёт выйти за границы
         * массива при любых обстоятельствах. */
        logger_flush_internal();
    }

    s_buffer[s_buffer_count].code  = code;
    s_buffer[s_buffer_count].value = value;
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

    return HAL_OK;
}

/* ------------------------------------------------------------------------ */
/*  Приём лога - главная горячая функция                                    */
/* ------------------------------------------------------------------------ */

void LOGGER_Log(uint32_t code, int32_t value)
{
    const LOGGER_LogEntry_t *entry = logger_find_entry(code);

    LOGGER_Priority_t priority   = (entry != NULL) ? entry->priority    : LOGGER_PRIORITY_HIGH;
    const char       *description = (entry != NULL) ? entry->description : NULL;

    if (s_config.console_fn != NULL)
    {
        s_config.console_fn(s_config.console_context, code, priority, value, description);
    }
    else
    {
        logger_swo_output(code, priority, value, description);
    }

    if (s_buffering_active && (entry != NULL))
    {
        logger_buffer_push(entry->code, entry->priority, value);
    }
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
