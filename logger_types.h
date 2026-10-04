/**
 ******************************************************************************
 * @file    logger_types.h
 * @brief   Портируемые типы формата лога и служебная таблица кодов самой
 *          библиотеки - общая часть встраиваемой и хостовой сторон.
 * @author  Mechanic
 * @date    04.10.2026
 * @version 1.9
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
#define LOGGER_INTERNAL_CODE_BUFFER_OVERFLOW  0x0004U /**< буфер физически полон, запись отброшена (см. README) */
#define LOGGER_INTERNAL_CODE_INIT_FAIL        0x0005U /**< LOGGER_Init() отклонена: source_id = код проблемной записи таблицы (0, если не относится к записи), value = LOGGER_INIT_FAIL_* */

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
    { LOGGER_INTERNAL_CODE_INIT,            LOGGER_PRIORITY_LOW,  "LOGGER: инициализация выполнена" },
    { LOGGER_INTERNAL_CODE_FLUSH,           LOGGER_PRIORITY_LOW,  "LOGGER: буфер сброшен в память" },
    { LOGGER_INTERNAL_CODE_MARK,            LOGGER_PRIORITY_LOW,  "LOGGER: временная метка" },
    { LOGGER_INTERNAL_CODE_BUFFER_OVERFLOW, LOGGER_PRIORITY_HIGH, "LOGGER: буфер полон, запись отброшена" },
    { LOGGER_INTERNAL_CODE_INIT_FAIL,       LOGGER_PRIORITY_HIGH, "LOGGER: Init отклонён, см. src/val" },
};

/** Количество записей в LOGGER_InternalTable. */
#define LOGGER_INTERNAL_TABLE_SIZE \
    ((uint32_t)(sizeof(LOGGER_InternalTable) / sizeof(LOGGER_InternalTable[0])))

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_TYPES_H */
