/**
 ******************************************************************************
 * @file    log_decoder.c
 * @brief   Реализация хостового декодера логов (см. log_decoder.h).
 * @author  Mechanic
 * @date    19.09.2026
 * @version 1.4
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

#include "log_decoder.h"
#include "logger_codes.h"
#include <string.h>

/* ------------------------------------------------------------------------ */
/*  Поиск записи в таблице кодов (двоичный поиск / служебная таблица)      */
/* ------------------------------------------------------------------------ */

/** Та же логика поиска, что и во встраиваемой части (logger.c) - таблица
 *  logger_codes.h гарантированно отсортирована по code по возрастанию, без
 *  повторов и без пересечения со служебным диапазоном (это условие
 *  проверяет LOGGER_Init() на встраиваемой стороне при сборке прошивки,
 *  один и тот же файл logger_codes.h используется на обеих сторонах). */
static const LOGGER_LogEntry_t *logdec_find_entry(uint16_t code)
{
    if (code <= LOGGER_INTERNAL_CODE_MAX)
    {
        for (size_t i = 0U; i < LOGGER_INTERNAL_TABLE_SIZE; i++)
        {
            if (LOGGER_InternalTable[i].code == code)
            {
                return &LOGGER_InternalTable[i];
            }
        }
        return NULL;
    }

    size_t lo = 0U;
    size_t hi = (size_t)LOGGER_LOG_TABLE_SIZE;

    while (lo < hi)
    {
        size_t   mid      = lo + ((hi - lo) >> 1U);
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
/*  Точечный поиск по коду                                                  */
/* ------------------------------------------------------------------------ */

const char *LOGDEC_GetDescription(uint16_t code)
{
    const LOGGER_LogEntry_t *entry = logdec_find_entry(code);
    return (entry != NULL) ? entry->description : NULL;
}

bool LOGDEC_GetPriority(uint16_t code, LOGGER_Priority_t *out_priority)
{
    if (out_priority == NULL)
    {
        return false;
    }

    const LOGGER_LogEntry_t *entry = logdec_find_entry(code);
    if (entry == NULL)
    {
        return false;
    }

    *out_priority = entry->priority;
    return true;
}

/* ------------------------------------------------------------------------ */
/*  Разбор дампа памяти                                                     */
/* ------------------------------------------------------------------------ */

bool LOGDEC_Decode(const uint8_t *data, size_t data_length,
                    LOGDEC_DecodedRecord_t *out_records, size_t out_capacity,
                    size_t *out_count)
{
    if ((data == NULL) || (out_records == NULL))
    {
        return false;
    }

    if ((data_length % sizeof(LOGGER_Record_t)) != 0U)
    {
        return false; /* дамп повреждён либо обрезан не по границе записи */
    }

    size_t record_count = data_length / sizeof(LOGGER_Record_t);
    if (record_count > out_capacity)
    {
        return false; /* буфер результата слишком мал, чтобы вместить всё - см. архитектуру, п.3 */
    }

    for (size_t i = 0U; i < record_count; i++)
    {
        /* Копия через memcpy, а не приведение (const LOGGER_Record_t*)data -
         * не предполагает выравнивания указателя data под LOGGER_Record_t и
         * не нарушает strict aliasing (data - это "просто байты" с точки
         * зрения компилятора). */
        LOGGER_Record_t raw;
        memcpy(&raw, data + (i * sizeof(LOGGER_Record_t)), sizeof(LOGGER_Record_t));

        const LOGGER_LogEntry_t *entry = logdec_find_entry(raw.code);

        out_records[i].code       = raw.code;
        out_records[i].source_id  = raw.source_id;
        out_records[i].value      = raw.value;
        out_records[i].systick    = raw.systick;
        out_records[i].rtc_time   = raw.rtc_time;
        out_records[i].priority    = (entry != NULL) ? entry->priority    : LOGGER_PRIORITY_HIGH;
        out_records[i].description = (entry != NULL) ? entry->description : NULL;
    }

    if (out_count != NULL)
    {
        *out_count = record_count;
    }

    return true;
}
