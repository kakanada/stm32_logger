/**
 ******************************************************************************
 * @file    log_decoder.h
 * @brief   Хостовая (ПК/сервер) часть библиотеки логирования - разбор дампа
 *          памяти обратно в записи. Полное описание API - см. README.md/
 *          API_REFERENCE.md.
 * @author  Mechanic
 * @date    19.09.2026
 * @version 1.7
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

#ifndef LOG_DECODER_H
#define LOG_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "logger_types.h"

/* ------------------------------------------------------------------------ */
/*  Декодированная запись - результат разбора одной LOGGER_Record_t         */
/* ------------------------------------------------------------------------ */

typedef struct
{
    uint16_t code;       /**< код лога, как в дампе памяти */
    uint16_t source_id;   /**< идентификатор источника события */
    int32_t  value;       /**< значение переменной */
    uint32_t systick;     /**< SysTick (HAL_GetTick() на встраиваемой стороне) на момент события */
    uint32_t rtc_time;    /**< показание RTC на момент события, либо 0 */

    /** Приоритет из таблицы кодов. Если code не найден ни в пользовательской,
     *  ни в служебной таблице - LOGGER_PRIORITY_HIGH (та же трактовка "по
     *  умолчанию", что и во встраиваемой части при выводе неизвестного кода). */
    LOGGER_Priority_t priority;

    /** Текстовое описание из таблицы кодов, либо NULL, если code не найден
     *  ни в пользовательской, ни в служебной таблице (рассинхронизация
     *  logger_codes.h между версией прошивки и версией декодера). */
    const char *description;
} LOGDEC_DecodedRecord_t;

/* ------------------------------------------------------------------------ */
/*  Разбор дампа памяти                                                     */
/* ------------------------------------------------------------------------ */

/**
 * @brief  Разбирает сырой дамп энергонезависимой памяти (массив
 *         LOGGER_Record_t подряд, ровно в том виде, в котором встраиваемая
 *         часть передавала его в write_fn) целиком в предоставленный буфер
 *         декодированных записей.
 *
 *         "Всё или ничего": если out_capacity не хватает, чтобы вместить все
 *         записи дампа, функция возвращает false и НЕ пишет в out_records.
 *
 * @param  data          указатель на сырые байты дампа
 * @param  data_length   длина data в байтах
 * @param  out_records   буфер результата, предоставляется и принадлежит
 *                        вызывающей стороне (без malloc внутри декодера)
 * @param  out_capacity  ёмкость out_records, в элементах (не в байтах)
 * @param  out_count     (опционально, может быть NULL) - сюда записывается
 *                        фактическое количество разобранных записей при
 *                        успехе; не трогается при неудаче
 * @retval true  - дамп полностью и успешно разобран, out_records заполнен
 * @retval false - data/out_records == NULL, data_length не кратна
 *                  sizeof(LOGGER_Record_t) (повреждённый/обрезанный дамп),
 *                  либо out_capacity не хватает на все записи дампа
 */
bool LOGDEC_Decode(const uint8_t *data, size_t data_length,
                    LOGDEC_DecodedRecord_t *out_records, size_t out_capacity,
                    size_t *out_count);

/* ------------------------------------------------------------------------ */
/*  Точечный поиск по коду                                                  */
/* ------------------------------------------------------------------------ */

/**
 * @brief  Возвращает текстовое описание кода лога - из пользовательской
 *         таблицы (logger_codes.h) либо, для диапазона 0x0000-0x00FF, из
 *         служебной таблицы библиотеки (LOGGER_InternalTable).
 * @param  code  код лога
 * @retval указатель на строку описания; NULL, если код не найден ни в одной
 *         из таблиц
 */
const char *LOGDEC_GetDescription(uint16_t code);

/**
 * @brief  Возвращает приоритет кода лога - из тех же таблиц, что и
 *         LOGDEC_GetDescription().
 * @param  code           код лога
 * @param  out_priority   куда записать результат
 * @retval true  - код найден, *out_priority заполнен
 * @retval false - out_priority == NULL либо код не найден ни в одной таблице
 */
bool LOGDEC_GetPriority(uint16_t code, LOGGER_Priority_t *out_priority);

/* ------------------------------------------------------------------------ */
/*  Постфактум-статистика частоты вызовов по разобранному дампу             */
/* ------------------------------------------------------------------------ */

/** Результат агрегации по одному коду - см. LOGDEC_ComputeCodeStats(). */
typedef struct
{
    uint16_t code;          /**< код лога */
    uint32_t call_count;    /**< сколько раз встретился в разобранном диапазоне */
    uint32_t first_systick;  /**< systick первой встреченной записи с этим кодом */
    uint32_t last_systick;   /**< systick последней встреченной записи с этим кодом */

    /** Средняя частота (вызовов/сек) между первой и последней записью этого
     *  кода. 0.0f, если call_count < 2 либо first_systick == last_systick
     *  (недостаточно данных для интервала - см. те же условия у
     *  LOGGER_GetCodeFrequency() на встраиваемой стороне). */
    float frequency_hz;
} LOGDEC_CodeStats_t;

/**
 * @brief  Считает статистику частоты вызовов по КАЖДОМУ отдельному коду,
 *         встретившемуся в records (кроме LOGGER_INTERNAL_CODE_MARK - см.
 *         LOGDEC_ComputeMarkStats() для меток). Хостовый аналог
 *         LOGGER_GetCodeFrequency() - считается сразу по всему дампу/диапазону
 *         записей, а не в реальном времени.
 *
 *         "Всё или ничего", как и LOGDEC_Decode(): если out_capacity не
 *         хватает на все различные коды, встретившиеся в records - функция
 *         возвращает false и НЕ пишет в out_stats.
 *
 * @param  records       массив декодированных записей (результат LOGDEC_Decode())
 * @param  record_count  количество записей в records
 * @param  out_stats     буфер результата, предоставляется вызывающей стороной
 * @param  out_capacity  ёмкость out_stats, в элементах
 * @param  out_count     (опционально, может быть NULL) - сюда записывается
 *                        фактическое количество различных кодов при успехе
 * @retval true  - статистика посчитана, out_stats заполнен (в порядке первого
 *         появления кода в records)
 * @retval false - records/out_stats == NULL (при record_count > 0), либо
 *         out_capacity не хватает на все различные коды из records
 */
bool LOGDEC_ComputeCodeStats(const LOGDEC_DecodedRecord_t *records, size_t record_count,
                              LOGDEC_CodeStats_t *out_stats, size_t out_capacity,
                              size_t *out_count);

/** Результат агрегации по одной метке (mark_id) - см. LOGDEC_ComputeMarkStats(). */
typedef struct
{
    uint16_t mark_id;       /**< идентификатор метки (совпадает с source_id записи) */
    uint32_t call_count;    /**< сколько раз встретилась в разобранном диапазоне */
    uint32_t first_systick;  /**< systick первого вызова этой метки */
    uint32_t last_systick;   /**< systick последнего вызова этой метки */

    /** Средняя частота (вызовов/сек), 0.0f при недостаточных данных - см.
     *  LOGDEC_CodeStats_t.frequency_hz выше. */
    float frequency_hz;
} LOGDEC_MarkStats_t;

/**
 * @brief  Считает статистику частоты вызовов по КАЖДОЙ отдельной метке
 *         (LOGGER_Mark(mark_id)) - находит в records все записи с кодом
 *         LOGGER_INTERNAL_CODE_MARK и агрегирует их по source_id (= mark_id,
 *         см. LOGGER_Mark() в logger.h). Хостовый аналог
 *         LOGGER_GetMarkFrequency(), считается сразу по всему дампу/диапазону.
 *
 *         "Всё или ничего", как и LOGDEC_ComputeCodeStats().
 *
 * @param  records       массив декодированных записей (результат LOGDEC_Decode())
 * @param  record_count  количество записей в records
 * @param  out_stats     буфер результата, предоставляется вызывающей стороной
 * @param  out_capacity  ёмкость out_stats, в элементах
 * @param  out_count     (опционально, может быть NULL) - сюда записывается
 *                        фактическое количество различных меток при успехе
 * @retval true  - статистика посчитана, out_stats заполнен (в порядке первого
 *         появления mark_id в records)
 * @retval false - records/out_stats == NULL (при record_count > 0), либо
 *         out_capacity не хватает на все различные mark_id из records
 */
bool LOGDEC_ComputeMarkStats(const LOGDEC_DecodedRecord_t *records, size_t record_count,
                              LOGDEC_MarkStats_t *out_stats, size_t out_capacity,
                              size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif /* LOG_DECODER_H */
