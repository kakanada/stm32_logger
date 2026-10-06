/**
 ******************************************************************************
 * @file    logger_types.h
 * @brief   Портируемые типы формата лога и служебная таблица кодов самой
 *          библиотеки - общая часть встраиваемой и хостовой сторон.
 * @author  Mechanic
 * @date    04.10.2026
 * @version 1.13
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

#ifndef LOGGER_TYPES_H
#define LOGGER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ------------------------------------------------------------------------ */
/*  Приоритет лога - 3 уровня                                               */
/* ------------------------------------------------------------------------ */

typedef enum
{
    LOGGER_PRIORITY_LOW    = 0,  /**< низкий приоритет - как правило, реже пишется в память */
    LOGGER_PRIORITY_MEDIUM = 1,  /**< средний приоритет */
    LOGGER_PRIORITY_HIGH   = 2   /**< высокий приоритет - как правило, чаще пишется в память */
} LOGGER_Priority_t;

/** Количество уровней приоритета - используется для размера внутренних
 *  массивов (порогов) на встраиваемой стороне. Значения LOGGER_Priority_t
 *  всегда 0..LOGGER_PRIORITY_COUNT-1. */
#define LOGGER_PRIORITY_COUNT 3U

/* ------------------------------------------------------------------------ */
/*  Запись таблицы кодов логов - см. logger_codes.h                        */
/* ------------------------------------------------------------------------ */

typedef struct
{
    /** Уникальный 16-битный код лога, указывается в HEX (см. рекомендацию по
     *  нумерации в шапке logger_codes.h). Диапазон 0x0000-0x00FF занят
     *  служебными логами библиотеки - использовать нельзя (см. ниже).
     *  Таблица должна быть отсортирована по этому полю по возрастанию,
     *  без повторов. */
    uint16_t code;

    /** Приоритет данного кода лога - один из 3 уровней. */
    LOGGER_Priority_t priority;

    /** Короткое текстовое описание - только для вывода в консоль/SWO/на
     *  хостовой стороне, в энергонезависимую память не попадает.
     *  Рекомендуемая длина - 32-64 символа, жёсткий предел -
     *  LOGGER_MAX_DESCRIPTION_LENGTH. */
    const char *description;
} LOGGER_LogEntry_t;

/** Жёсткий предел длины текстового описания записи таблицы кодов (символов,
 *  без учёта завершающего '\0'). Рекомендуемая длина - 32-64 символа (см.
 *  logger_codes.h), проверяется в LOGGER_Init() на встраиваемой стороне. */
#define LOGGER_MAX_DESCRIPTION_LENGTH 64U

/* ------------------------------------------------------------------------ */
/*  Формат одной записи в буфере / в данных, переданных в write_fn / в       */
/*  дампе энергонезависимой памяти, который разбирает LOGDEC_Decode()       */
/* ------------------------------------------------------------------------ */

typedef struct
{
    uint16_t code;      /**< код лога (как в LOGGER_LogTable) */
    uint16_t source_id;  /**< идентификатор источника события, из LOGGER_Log() */
    int32_t  value;      /**< значение переменной, переданное в LOGGER_Log() */
    uint32_t systick;    /**< HAL_GetTick() на момент события, мс с момента старта */
    uint32_t rtc_time;   /**< показание RTC на момент события, либо 0 (см. rtc_time_fn) */
} LOGGER_Record_t;

/* ------------------------------------------------------------------------ */
/*  Текстовое представление rtc_time (Unix-время, секунды с 1970, UTC без   */
/*  часового пояса) - общее для встраиваемой и хостовой стороны              */
/* ------------------------------------------------------------------------ */

/** Ширина колонки RTC в текстовой строке лога: "ГГГГ-ММ-ДД ЧЧ:ММ:СС". */
#define LOGGER_RTC_TEXT_WIDTH 19U

/** @brief Переводит Unix-время в "ГГГГ-ММ-ДД ЧЧ:ММ:СС" (ровно 19 символов +
 *         '\0'). unix_time == 0 (RTC не используется) - 19 пробелов, чтобы
 *         колонки строки оставались ровными. Без календарных библиотек и
 *         malloc (алгоритм civil-from-days).
 * @param  unix_time секунды с 1970-01-01 00:00:00
 * @param  out       буфер минимум LOGGER_RTC_TEXT_WIDTH + 1 байт */
static inline void LOGGER_UnixTimeToText(uint32_t unix_time, char out[LOGGER_RTC_TEXT_WIDTH + 1U])
{
    if (unix_time == 0U)
    {
        for (uint32_t i = 0U; i < LOGGER_RTC_TEXT_WIDTH; i++)
        {
            out[i] = ' ';
        }
        out[LOGGER_RTC_TEXT_WIDTH] = '\0';
        return;
    }

    uint32_t days = unix_time / 86400U;
    uint32_t secs = unix_time % 86400U;
    uint32_t z    = days + 719468U;
    uint32_t era  = z / 146097U;
    uint32_t doe  = z - (era * 146097U);
    uint32_t yoe  = (doe - (doe / 1460U) + (doe / 36524U) - (doe / 146096U)) / 365U;
    uint32_t doy  = doe - ((365U * yoe) + (yoe / 4U) - (yoe / 100U));
    uint32_t mp   = ((5U * doy) + 2U) / 153U;
    uint32_t day  = doy - (((153U * mp) + 2U) / 5U) + 1U;
    uint32_t mon  = (mp < 10U) ? (mp + 3U) : (mp - 9U);
    uint32_t year = yoe + (era * 400U) + ((mon <= 2U) ? 1U : 0U);
    uint32_t v[6] = { year, mon, day, secs / 3600U, (secs % 3600U) / 60U, secs % 60U };
    static const uint8_t pos[6]   = { 0U, 5U, 8U, 11U, 14U, 17U };
    static const uint8_t digits[6] = { 4U, 2U, 2U, 2U, 2U, 2U };

    out[4] = '-'; out[7] = '-'; out[10] = ' '; out[13] = ':'; out[16] = ':';
    out[LOGGER_RTC_TEXT_WIDTH] = '\0';
    for (uint32_t f = 0U; f < 6U; f++)
    {
        uint32_t x = v[f];
        for (uint32_t d = digits[f]; d > 0U; d--)
        {
            out[pos[f] + d - 1U] = (char)('0' + (x % 10U));
            x /= 10U;
        }
    }
}
/* ------------------------------------------------------------------------ */
/*  Служебные (зарезервированные) коды логов самой библиотеки               */
/* ------------------------------------------------------------------------ */

/** Весь диапазон 0x0000-0x00FF (старший байт кода 0x00) зарезервирован под
 *  служебные логи библиотеки - LOGGER_Init() отклонит таблицу пользователя
 *  (logger_codes.h), если в ней есть коды из этого диапазона. Хостовый
 *  декодер (log_decoder.h) знает о них через LOGGER_InternalTable ниже - эти
 *  коды тоже могут встретиться в дампе памяти (например, LOGGER_Mark()
 *  буферизуется как обычный лог). */
#define LOGGER_INTERNAL_CODE_MIN 0x0000U
#define LOGGER_INTERNAL_CODE_MAX 0x00FFU

#define LOGGER_INTERNAL_CODE_INIT             0x0001U /**< LOGGER_Init() успешно завершена */
#define LOGGER_INTERNAL_CODE_FLUSH            0x0002U /**< буфер сброшен в write_fn */
#define LOGGER_INTERNAL_CODE_MARK             0x0003U /**< вызвана LOGGER_Mark() */
#define LOGGER_INTERNAL_CODE_BUFFER_OVERFLOW  0x0004U /**< кольцо физически полно, запись отброшена: source_id = код отброшенной записи, value = номер срабатывания (только консоль, см. README) */
#define LOGGER_INTERNAL_CODE_INIT_FAIL        0x0005U /**< LOGGER_Init() отклонена: source_id = код проблемной записи таблицы (0, если не относится к записи), value = LOGGER_INIT_FAIL_* */
#define LOGGER_INTERNAL_CODE_UNKNOWN_CODE     0x0006U /**< LOGGER_Log() вызван с кодом, которого нет в таблице: source_id = этот код, value = номер срабатывания */
#define LOGGER_INTERNAL_CODE_RING_CORRUPT     0x0007U /**< индексы кольцевого буфера повреждены, накопленное отброшено: source_id = число потерянных записей, value = номер срабатывания */
#define LOGGER_INTERNAL_CODE_WAIT_TIMEOUT     0x0008U /**< write_fn не освободил место за LOGGER_OVERFLOW_WAIT_MS: source_id = код отброшенной записи, value = номер срабатывания */
#define LOGGER_INTERNAL_CODE_MARK_ID_RANGE    0x0009U /**< LOGGER_Mark() с mark_id >= LOGGER_MARK_MAX_IDS (метка записана, статистика нет): source_id = mark_id, value = номер срабатывания */
#define LOGGER_INTERNAL_CODE_NOT_INIT         0x000AU /**< вызов API до LOGGER_Init() либо без write_fn: source_id = LOGGER_NOT_INIT_API_*, value = номер срабатывания */
#define LOGGER_INTERNAL_CODE_DROPPED_SUMMARY  0x000BU /**< сохраняется в память, когда после потерь снова появилось место: value = сколько записей потеряно с прошлой такой записи */

/** Какой API вызван в неподходящем состоянии - source_id служебного лога
 *  LOGGER_INTERNAL_CODE_NOT_INIT. */
#define LOGGER_NOT_INIT_API_LOG             1U
#define LOGGER_NOT_INIT_API_MARK            2U
#define LOGGER_NOT_INIT_API_FLUSH           3U
#define LOGGER_NOT_INIT_API_EMERGENCY_SAVE  4U

/** Причины отказа LOGGER_Init() - передаются в value служебного лога
 *  LOGGER_INTERNAL_CODE_INIT_FAIL (выводится только в console_fn/SWO). */
#define LOGGER_INIT_FAIL_CONFIG_NULL        1 /**< config == NULL */
#define LOGGER_INIT_FAIL_TABLE_EMPTY        2 /**< пользовательская таблица пуста */
#define LOGGER_INIT_FAIL_CODE_RESERVED      3 /**< код из служебного диапазона 0x0000-0x00FF */
#define LOGGER_INIT_FAIL_BAD_PRIORITY       4 /**< некорректный приоритет записи */
#define LOGGER_INIT_FAIL_DESC_NULL          5 /**< description == NULL */
#define LOGGER_INIT_FAIL_DESC_TOO_LONG      6 /**< описание длиннее LOGGER_MAX_DESCRIPTION_LENGTH байт */
#define LOGGER_INIT_FAIL_NOT_SORTED         7 /**< таблица не отсортирована либо повтор кода */
#define LOGGER_INIT_FAIL_BAD_THRESHOLD      8 /**< порог 0 либо больше LOGGER_BUFFER_CAPACITY */
#define LOGGER_INIT_FAIL_CONSOLE_REQUIRED   9 /**< LOGGER_NO_ITM без console_fn */

/** Таблица служебных кодов библиотеки - используется и встраиваемой стороной
 *  (logger.c, для поиска приоритета/описания кодов 0x0000-0x00FF), и
 *  хостовым декодером (log_decoder.c, по тем же причинам, при разборе дампа
 *  памяти). "static const" - как и LOGGER_LogTable в logger_codes.h, чтобы
 *  подключение этого заголовка в разные единицы трансляции не приводило к
 *  конфликту символов на этапе линковки. */
static const LOGGER_LogEntry_t LOGGER_InternalTable[] =
{
    { LOGGER_INTERNAL_CODE_INIT,            LOGGER_PRIORITY_LOW,  "инициализация выполнена" },
    { LOGGER_INTERNAL_CODE_FLUSH,           LOGGER_PRIORITY_LOW,  "буфер сброшен в память" },
    { LOGGER_INTERNAL_CODE_MARK,            LOGGER_PRIORITY_LOW,  "временная метка" },
    { LOGGER_INTERNAL_CODE_BUFFER_OVERFLOW, LOGGER_PRIORITY_HIGH, "буфер полон, запись отброшена" },
    { LOGGER_INTERNAL_CODE_INIT_FAIL,       LOGGER_PRIORITY_HIGH, "Init отклонён, см. src/val" },
    { LOGGER_INTERNAL_CODE_UNKNOWN_CODE,    LOGGER_PRIORITY_HIGH,   "неизвестный код лога (src=код)" },
    { LOGGER_INTERNAL_CODE_RING_CORRUPT,    LOGGER_PRIORITY_HIGH,   "кольцо повреждено (src=потеряно)" },
    { LOGGER_INTERNAL_CODE_WAIT_TIMEOUT,    LOGGER_PRIORITY_HIGH,   "write_fn завис, отброшено (src=код)" },
    { LOGGER_INTERNAL_CODE_MARK_ID_RANGE,   LOGGER_PRIORITY_MEDIUM, "mark_id вне диапазона (src=mark_id)" },
    { LOGGER_INTERNAL_CODE_NOT_INIT,        LOGGER_PRIORITY_MEDIUM, "вызов до Init либо без write_fn (src=API)" },
    { LOGGER_INTERNAL_CODE_DROPPED_SUMMARY, LOGGER_PRIORITY_HIGH,   "потеряно записей с прошлой сводки" },
};

/** Количество записей в LOGGER_InternalTable. */
#define LOGGER_INTERNAL_TABLE_SIZE \
    ((uint32_t)(sizeof(LOGGER_InternalTable) / sizeof(LOGGER_InternalTable[0])))

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_TYPES_H */
