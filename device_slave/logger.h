/**
 ******************************************************************************
 * @file    logger.h
 * @brief   Библиотека логирования для STM32, роль device_slave: второстепенный
 *          МК без собственного хранилища логов. Выводит логи в SWO/консоль и
 *          отправляет их master по любому интерфейсу через функцию-указатель.
 *          Полное описание - см. README.md/API_REFERENCE.md.
 * @author  Mechanic
 * @date    13.10.2026
 * @version 2.0
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

#ifndef LOGGER_H
#define LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "main.h"          /* CubeMX: HAL_StatusTypeDef, HAL_GetTick() */
#include "logger_types.h"  /* типы и формат кадра, общие с master и host */

/* ------------------------------------------------------------------------ */
/*  Конфигурация модуля (define-ы, меняются до включения заголовка)         */
/* ------------------------------------------------------------------------ */

/** Ёмкость кольца кадров на отправку (без malloc). Одна запись - 12 байт ОЗУ.
 *  Диапазон 2..255. Уменьшайте для МК с малым ОЗУ. */
#ifndef LOGGER_BUFFER_CAPACITY
#define LOGGER_BUFFER_CAPACITY 32U
#endif

/** Максимум кадров, отправляемых за один вызов LOGGER_Process() - ограничивает
 *  нагрузку на шину и время в главном цикле (защита от лавины). */
#ifndef LOGGER_MAX_FRAMES_PER_PROCESS
#define LOGGER_MAX_FRAMES_PER_PROCESS 4U
#endif

/* ------------------------------------------------------------------------ */
/*  Пользовательские функции                                                */
/* ------------------------------------------------------------------------ */

/**
 * @brief  Функция отправки одного кадра master по физическому интерфейсу
 *         (CAN, UART, SPI, I2C - библиотеке всё равно), предоставляется
 *         пользователем. Вызывается из LOGGER_Process() (контекст main), не из
 *         LOGGER_Log(). Не должна блокироваться надолго.
 *         Как уложить кадр в интерфейс - выбор пользователя. Хелперы:
 *         LOGGER_FramePackRaw() (10 байт), LOGGER_FramePackStream() (13 байт,
 *         SYNC + CRC16, для UART/SPI). Для classic CAN: идентификатор
 *         (расширенный) = (node_id << 8) | seq, данные 8 байт = code(2),
 *         source_id(2), value(4), little-endian.
 * @param  context значение send_context из LOGGER_Init()
 * @param  frame   кадр - только на время вызова
 * @retval HAL_OK - кадр принят интерфейсом (удаляется из очереди); любое
 *         другое значение (например HAL_BUSY, нет свободного почтового ящика) -
 *         кадр остаётся в очереди и будет отправлен при следующем
 *         LOGGER_Process().
 */
typedef HAL_StatusTypeDef (*LOGGER_SendFn_t)(void *context, const LOGGER_Frame_t *frame);

/**
 * @brief  Функция вывода одной записи в консоль (UART/COM и т.п.), заменяет
 *         вывод по умолчанию в SWO. Вызывается на каждый LOGGER_Log() сразу.
 *         Аргументы те же, что у master (rtc_time всегда 0, node_id - id этого slave).
 */
typedef void (*LOGGER_ConsoleWriteFn_t)(void *context, uint8_t node_id, uint16_t code, uint16_t source_id,
                                         LOGGER_Priority_t priority, int32_t value,
                                         uint32_t systick, uint32_t rtc_time,
                                         const char *description);

/* ------------------------------------------------------------------------ */
/*  Конфигурация инициализации                                              */
/* ------------------------------------------------------------------------ */

typedef struct
{
    /** Id этого slave на линии, 1..255 (у каждого slave свой). */
    uint8_t node_id;

    /** Функция отправки кадра master. NULL - отправка выключена, только вывод. */
    LOGGER_SendFn_t send_fn;

    /** Указатель, передаваемый в send_fn первым аргументом. Может быть NULL. */
    void *send_context;

    /** Минимальный приоритет записи, отправляемой master: LOW (0, по
     *  умолчанию) - всё; MEDIUM; HIGH - только ошибки. Позволяет не грузить шину. */
    LOGGER_Priority_t send_min_priority;

    /** Функция вывода в консоль. NULL - вывод по умолчанию в SWO (ITM). */
    LOGGER_ConsoleWriteFn_t console_fn;

    /** Указатель, передаваемый в console_fn первым аргументом. Может быть NULL. */
    void *console_context;
} LOGGER_Config_t;

/* ------------------------------------------------------------------------ */
/*  API                                                                     */
/* ------------------------------------------------------------------------ */

/**
 * @brief  Инициализирует slave: проверяет конфигурацию, обнуляет очередь и
 *         нумерацию кадров. Необязательна: без неё LOGGER_Log() выводит только в SWO.
 * @param  config конфигурация
 * @retval HAL_OK; HAL_ERROR - config == NULL, node_id == 0, неверный приоритет
 *         либо (при LOGGER_NO_ITM) не задана console_fn
 */
HAL_StatusTypeDef LOGGER_Init(const LOGGER_Config_t *config);

/**
 * @brief  Логирует событие: сразу выводит в консоль/SWO и ставит кадр в
 *         очередь на отправку master. Быстрая, неблокирующая, из любого
 *         контекста (ISR допустим). Если очередь полна - кадр теряется, потеря
 *         видна master по разрыву seq и по служебной записи
 *         LOGGER_INTERNAL_CODE_DROPPED_SUMMARY (value = сколько потеряно).
 * @param  code      код лога (logger_codes.h)
 * @param  source_id идентификатор источника события или 0
 * @param  value     значение переменной
 */
void LOGGER_Log(uint16_t code, uint16_t source_id, int32_t value);

/**
 * @brief  Отправляет накопленные кадры master через send_fn (не более
 *         LOGGER_MAX_FRAMES_PER_PROCESS за вызов). Вызывать регулярно из
 *         главного цикла. Если send_fn вернула не HAL_OK - останавливается до
 *         следующего вызова.
 * @return сколько кадров отправлено за этот вызов
 */
uint8_t LOGGER_Process(void);

/**
 * @brief  Сколько кадров потеряно локально (очередь была полна) с момента старта.
 * @return число потерь. Безопасно из любого контекста.
 */
uint32_t LOGGER_GetDroppedCount(void);

/**
 * @brief  Сколько кадров сейчас ждёт отправки.
 * @return число кадров в очереди
 */
uint16_t LOGGER_GetPending(void);

/**
 * @brief  Формирует строку лога (формат как у master, без добивки) в буфер.
 * @param  buf  буфер результата (может быть NULL при size == 0)
 * @param  size размер buf
 * @param  node_id 0 - master, 1..255 - slave
 * @return длина полной строки без '\0' (как snprintf)
 *         остальные аргументы - как у console_fn
 */
size_t LOGGER_FormatConsoleLine(char *buf, size_t size, uint8_t node_id, uint16_t code, uint16_t source_id,
                                 LOGGER_Priority_t priority, int32_t value, uint32_t systick,
                                 uint32_t rtc_time, const char *description);

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
