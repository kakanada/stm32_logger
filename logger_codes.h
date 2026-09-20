/**
 ******************************************************************************
 * @file    logger_codes.h
 * @brief   Таблица кодов логов проекта - коды, приоритеты, текстовые описания.
 *          Заполняется индивидуально под каждый проект, общая для клиентской
 *          и хостовой сторон библиотеки.
 * @author  Mechanic
 * @date    19.09.2026
 * @version 1.7
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

/* Полное описание правил нумерации кодов и интеграции зависимых библиотек -
 * см. README.md/API_REFERENCE.md. Кратко: старший байт кода = адресное
 * пространство (4 категории - система/периферия/устройства/резерв), младший
 * байт = номер события; каждая группа - триада define LOG_ADDR_/LOG_OFFSET_/
 * LOG_CODE_<ИМЯ>_<СОБЫТИЕ>; зависимая библиотека получает свой блок под
 * "#ifdef LOGGER_ENABLE_<ИМЯ>" только после подтверждения полного списка
 * событий; диапазон 0x0000-0x00FF зарезервирован под служебные логи
 * библиотеки (см. logger_types.h). */

#ifndef LOGGER_CODES_H
#define LOGGER_CODES_H

#include "logger_types.h"

/* ============================================================================
 *  СПРАВКА - все известные подключаемые зависимые библиотеки этого проекта
 * ============================================================================
 *  Каждая - переключатель "#define LOGGER_ENABLE_<ИМЯ>" перед
 *  "#include \"logger_codes.h\"" в проекте, который реально её использует.
 *  Если библиотека проекту не нужна - соответствующий define просто не
 *  ставится, и ни один байт под её коды/записи таблицы не попадает в сборку
 *  (см. правила интеграции выше). Список ведётся здесь для удобства - чтобы
 *  видеть все резервирования сразу, без прокрутки всего файла:
 *
 *      LOGGER_ENABLE_CANMGR      - can_manager                  (0x41, периферия/CAN)
 *      LOGGER_ENABLE_LOAD_PWM    - stm32_load_pwm                (0x81, устройство)
 *      LOGGER_ENABLE_RC_BUS      - rc_bus + rc_bus_telemetry      (0x82, устройство)
 *      LOGGER_ENABLE_VESC_SERVO  - can_vesc_servo_stm32           (0x83, устройство)
 *      LOGGER_ENABLE_VESC        - can_vesc_stm32 (motor_vesc)    (0x84, устройство)
 *      LOGGER_ENABLE_BISS_IRS    - stm32_biss_irs                 (0x85, устройство)
 *      LOGGER_ENABLE_LSM6DSX     - lsm6dsx (accel+gyro)           (0x86, устройство)
 * ============================================================================
 */

/* ---------------------------------------------------------------------- */
/*  Адресные пространства и коды - и собственных групп проекта (ниже,     */
/*  без #ifdef - они часть базового примера таблицы), и зависимых         */
/*  библиотек (каждая под своим "#ifdef LOGGER_ENABLE_<ИМЯ>"). Всё         */
/*  определяется ДО таблицы LOGGER_LogTable, чтобы использоваться прямо   */
/*  в её инициализаторе. Смена диапазона любой группы/библиотеки - правка */
/*  одного define LOG_ADDR_<ИМЯ>, без изменений где-либо ещё.             */
/*                                                                        */
/*  Ниже группы идут по категориям (см. шапку файла):                     */
/*    1) СИСТЕМА / внутренние алгоритмы  - 0x00xx-0x3Fxx                  */
/*    2) ПЕРИФЕРИЯ (интерфейсы)          - 0x40xx-0x7Fxx                  */
/*    3) ВНЕШНИЕ УСТРОЙСТВА/ДАТЧИКИ      - 0x80xx-0xDFxx                  */
/*    4) РЕЗЕРВ                          - 0xE0xx-0xFFxx (пока не занят)  */
/* ---------------------------------------------------------------------- */

/* ======================================================================== */
/*  КАТЕГОРИЯ 1/4 - СИСТЕМА и внутренние алгоритмы (0x00xx-0x3Fxx)          */
/* ======================================================================== */

/* ------------------------------------------------------------------ */
/*  ПРИМЕР - собственная группа проекта (замените на реальную).       */
/* ------------------------------------------------------------------ */
/* Адресное пространство группы "система". */
#define LOG_ADDR_SYSTEM 0x01U
#define LOG_OFFSET_SYSTEM_START     0x01U
#define LOG_OFFSET_SYSTEM_HAL_ERROR 0x02U
#define LOG_CODE_SYSTEM_START     ((uint16_t)((LOG_ADDR_SYSTEM << 8) | LOG_OFFSET_SYSTEM_START))
#define LOG_CODE_SYSTEM_HAL_ERROR ((uint16_t)((LOG_ADDR_SYSTEM << 8) | LOG_OFFSET_SYSTEM_HAL_ERROR))

/* ======================================================================== */
/*  КАТЕГОРИЯ 2/4 - ПЕРИФЕРИЯ, интерфейсы МК (0x40xx-0x7Fxx)                */
/* ======================================================================== */

/* ПРИМЕР (не компилируется - просто иллюстрация паттерна, см. правила выше):
 *   #define LOG_ADDR_SPI 0x40U
 *   #define LOG_OFFSET_SPI_RETRY 0x01U
 *   #define LOG_CODE_SPI_RETRY ((uint16_t)((LOG_ADDR_SPI << 8) | LOG_OFFSET_SPI_RETRY))
 * Коды интерфейсов/периферии заводятся здесь ТОЛЬКО когда есть реальная
 * зависимая библиотека, полностью подтвердившая свой список событий (как
 * CANMGR ниже) - "заглушек про запас" в этой категории не держим. */

#ifdef LOGGER_ENABLE_CANMGR
/* Адресное пространство (старший байт кода), выделенное can_manager. */
#define LOG_ADDR_CANMGR 0x41U

/* Смещения (младший байт) - номер события внутри диапазона can_manager. */
#define LOG_OFFSET_CANMGR_INIT_OK       0x00U
#define LOG_OFFSET_CANMGR_INIT_FAIL     0x01U
#define LOG_OFFSET_CANMGR_REG_REJECTED  0x02U
#define LOG_OFFSET_CANMGR_TX_QUEUE_FULL 0x03U
#define LOG_OFFSET_CANMGR_RX_OVERFLOW   0x04U
#define LOG_OFFSET_CANMGR_BUS_OFF       0x05U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом can_manager. */
#define LOG_CODE_CANMGR_INIT_OK       ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_INIT_OK))
#define LOG_CODE_CANMGR_INIT_FAIL     ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_INIT_FAIL))
#define LOG_CODE_CANMGR_REG_REJECTED  ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_REG_REJECTED))
#define LOG_CODE_CANMGR_TX_QUEUE_FULL ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_TX_QUEUE_FULL))
#define LOG_CODE_CANMGR_RX_OVERFLOW   ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_RX_OVERFLOW))
#define LOG_CODE_CANMGR_BUS_OFF       ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_BUS_OFF))
#endif /* LOGGER_ENABLE_CANMGR */

/* ======================================================================== */
/*  КАТЕГОРИЯ 3/4 - ВНЕШНИЕ УСТРОЙСТВА/ДАТЧИКИ на периферии (0x80xx-0xDFxx) */
/* ======================================================================== */

/* ПРИМЕР (не компилируется - просто иллюстрация паттерна, см. правила выше):
 *   #define LOG_ADDR_MEMORY 0x80U
 *   #define LOG_OFFSET_MEMORY_WRITE_TIMEOUT 0x01U
 *   #define LOG_CODE_MEMORY_WRITE_TIMEOUT ((uint16_t)((LOG_ADDR_MEMORY << 8) | LOG_OFFSET_MEMORY_WRITE_TIMEOUT))
 * Например, будущая зависимая библиотека внешней флеш-памяти (W25Qxx и т.п.)
 * получит здесь свой блок под "#ifdef LOGGER_ENABLE_<ИМЯ>" по тому же
 * образцу, что и LOAD_PWM ниже - ТОЛЬКО после того, как она подтвердит
 * полный список своих кодов/приоритетов/описаний. Никаких кодов "про
 * запас" в этой категории не держим. */

#ifdef LOGGER_ENABLE_LOAD_PWM
/* Адресное пространство (старший байт кода), выделенное LOAD_PWM. */
#define LOG_ADDR_LOAD_PWM 0x81U

/* Смещения (младший байт) - номер события внутри диапазона LOAD_PWM. */
#define LOG_OFFSET_LOAD_PWM_INIT_BAD_CONFIG    0x00U
#define LOG_OFFSET_LOAD_PWM_INIT_POOL_FULL     0x01U
#define LOG_OFFSET_LOAD_PWM_INIT_LED_POOL_FULL 0x02U
#define LOG_OFFSET_LOAD_PWM_INIT_HAL_START_FAIL 0x03U
#define LOG_OFFSET_LOAD_PWM_INIT_OK             0x04U
#define LOG_OFFSET_LOAD_PWM_CYCLE_START         0x05U
#define LOG_OFFSET_LOAD_PWM_CYCLE_DONE          0x06U
#define LOG_OFFSET_LOAD_PWM_STOP                0x07U
#define LOG_OFFSET_LOAD_PWM_STOP_ALL             0x08U
#define LOG_OFFSET_LOAD_PWM_NULL_HANDLE          0x09U
#define LOG_OFFSET_LOAD_PWM_GLOBAL_BRIGHTNESS    0x0AU

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самой LOAD_PWM. */
#define LOG_CODE_LOAD_PWM_INIT_BAD_CONFIG     ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_INIT_BAD_CONFIG))
#define LOG_CODE_LOAD_PWM_INIT_POOL_FULL      ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_INIT_POOL_FULL))
#define LOG_CODE_LOAD_PWM_INIT_LED_POOL_FULL  ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_INIT_LED_POOL_FULL))
#define LOG_CODE_LOAD_PWM_INIT_HAL_START_FAIL ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_INIT_HAL_START_FAIL))
#define LOG_CODE_LOAD_PWM_INIT_OK             ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_INIT_OK))
#define LOG_CODE_LOAD_PWM_CYCLE_START         ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_CYCLE_START))
#define LOG_CODE_LOAD_PWM_CYCLE_DONE          ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_CYCLE_DONE))
#define LOG_CODE_LOAD_PWM_STOP                ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_STOP))
#define LOG_CODE_LOAD_PWM_STOP_ALL            ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_STOP_ALL))
#define LOG_CODE_LOAD_PWM_NULL_HANDLE         ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_NULL_HANDLE))
#define LOG_CODE_LOAD_PWM_GLOBAL_BRIGHTNESS   ((uint16_t)((LOG_ADDR_LOAD_PWM << 8) | LOG_OFFSET_LOAD_PWM_GLOBAL_BRIGHTNESS))
#endif /* LOGGER_ENABLE_LOAD_PWM */

#ifdef LOGGER_ENABLE_RC_BUS
/* Адресное пространство (старший байт кода), выделенное rc_bus (общее для
 * rc_bus.c и rc_bus_telemetry.c). */
#define LOG_ADDR_RC_BUS 0x82U

/* Смещения (младший байт) - номер события внутри диапазона rc_bus. */
#define LOG_OFFSET_RC_BUS_INIT_FAIL              0x00U
#define LOG_OFFSET_RC_BUS_UART_ERROR              0x01U
#define LOG_OFFSET_RC_BUS_FRAME_ERROR             0x02U
#define LOG_OFFSET_RC_BUS_TELEMETRY_INIT_FAIL     0x03U
#define LOG_OFFSET_RC_BUS_TELEMETRY_UART_ERROR    0x04U
#define LOG_OFFSET_RC_BUS_TELEMETRY_FRAME_ERROR   0x05U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом rc_bus. */
#define LOG_CODE_RC_BUS_INIT_FAIL            ((uint16_t)((LOG_ADDR_RC_BUS << 8) | LOG_OFFSET_RC_BUS_INIT_FAIL))
#define LOG_CODE_RC_BUS_UART_ERROR            ((uint16_t)((LOG_ADDR_RC_BUS << 8) | LOG_OFFSET_RC_BUS_UART_ERROR))
#define LOG_CODE_RC_BUS_FRAME_ERROR           ((uint16_t)((LOG_ADDR_RC_BUS << 8) | LOG_OFFSET_RC_BUS_FRAME_ERROR))
#define LOG_CODE_RC_BUS_TELEMETRY_INIT_FAIL   ((uint16_t)((LOG_ADDR_RC_BUS << 8) | LOG_OFFSET_RC_BUS_TELEMETRY_INIT_FAIL))
#define LOG_CODE_RC_BUS_TELEMETRY_UART_ERROR  ((uint16_t)((LOG_ADDR_RC_BUS << 8) | LOG_OFFSET_RC_BUS_TELEMETRY_UART_ERROR))
#define LOG_CODE_RC_BUS_TELEMETRY_FRAME_ERROR ((uint16_t)((LOG_ADDR_RC_BUS << 8) | LOG_OFFSET_RC_BUS_TELEMETRY_FRAME_ERROR))
#endif /* LOGGER_ENABLE_RC_BUS */

#ifdef LOGGER_ENABLE_VESC_SERVO
/* Адресное пространство (старший байт кода), выделенное vesc_servo
 * (can_vesc_servo_stm32). */
#define LOG_ADDR_VESC_SERVO 0x83U

/* Смещения (младший байт) - номер события внутри диапазона vesc_servo. */
#define LOG_OFFSET_VESC_SERVO_INIT_BAD_CONFIG 0x00U
#define LOG_OFFSET_VESC_SERVO_INIT_POOL_FULL  0x01U
#define LOG_OFFSET_VESC_SERVO_INIT_VESC_FAIL  0x02U
#define LOG_OFFSET_VESC_SERVO_INIT_DUPLICATE  0x03U
#define LOG_OFFSET_VESC_SERVO_INIT_OK         0x04U
#define LOG_OFFSET_VESC_SERVO_FAULT_ENTERED   0x05U
#define LOG_OFFSET_VESC_SERVO_HOMING_START    0x06U
#define LOG_OFFSET_VESC_SERVO_HOMING_DONE     0x07U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом vesc_servo. */
#define LOG_CODE_VESC_SERVO_INIT_BAD_CONFIG ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_INIT_BAD_CONFIG))
#define LOG_CODE_VESC_SERVO_INIT_POOL_FULL  ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_INIT_POOL_FULL))
#define LOG_CODE_VESC_SERVO_INIT_VESC_FAIL  ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_INIT_VESC_FAIL))
#define LOG_CODE_VESC_SERVO_INIT_DUPLICATE  ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_INIT_DUPLICATE))
#define LOG_CODE_VESC_SERVO_INIT_OK         ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_INIT_OK))
#define LOG_CODE_VESC_SERVO_FAULT_ENTERED   ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_FAULT_ENTERED))
#define LOG_CODE_VESC_SERVO_HOMING_START    ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_HOMING_START))
#define LOG_CODE_VESC_SERVO_HOMING_DONE     ((uint16_t)((LOG_ADDR_VESC_SERVO << 8) | LOG_OFFSET_VESC_SERVO_HOMING_DONE))
#endif /* LOGGER_ENABLE_VESC_SERVO */

#ifdef LOGGER_ENABLE_VESC
/* Адресное пространство (старший байт кода), выделенное motor_vesc
 * (can_vesc_stm32). */
#define LOG_ADDR_VESC 0x84U

/* Смещения (младший байт) - номер события внутри диапазона motor_vesc. */
#define LOG_OFFSET_VESC_INIT_OK        0x00U
#define LOG_OFFSET_VESC_INIT_FAIL      0x01U
#define LOG_OFFSET_VESC_REG_REJECTED   0x02U
#define LOG_OFFSET_VESC_EXIST_TIMEOUT  0x03U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом motor_vesc. */
#define LOG_CODE_VESC_INIT_OK       ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_INIT_OK))
#define LOG_CODE_VESC_INIT_FAIL     ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_INIT_FAIL))
#define LOG_CODE_VESC_REG_REJECTED  ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_REG_REJECTED))
#define LOG_CODE_VESC_EXIST_TIMEOUT ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_EXIST_TIMEOUT))
#endif /* LOGGER_ENABLE_VESC */

#ifdef LOGGER_ENABLE_BISS_IRS
/* Адресное пространство (старший байт кода), выделенное stm32_biss_irs. */
#define LOG_ADDR_BISS_IRS 0x85U

/* Смещения (младший байт) - номер события внутри диапазона stm32_biss_irs. */
#define LOG_OFFSET_BISS_IRS_INIT_BAD_CONFIG    0x00U
#define LOG_OFFSET_BISS_IRS_INIT_POOL_FULL     0x01U
#define LOG_OFFSET_BISS_IRS_INIT_CLOCK_RANGE   0x02U
#define LOG_OFFSET_BISS_IRS_INIT_OK            0x03U
#define LOG_OFFSET_BISS_IRS_POLL_ACK_TIMEOUT   0x04U
#define LOG_OFFSET_BISS_IRS_POLL_FRAME_INVALID 0x05U
#define LOG_OFFSET_BISS_IRS_ZERO_HERE_REJECTED 0x06U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом stm32_biss_irs. */
#define LOG_CODE_BISS_IRS_INIT_BAD_CONFIG    ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_BAD_CONFIG))
#define LOG_CODE_BISS_IRS_INIT_POOL_FULL     ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_POOL_FULL))
#define LOG_CODE_BISS_IRS_INIT_CLOCK_RANGE   ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_CLOCK_RANGE))
#define LOG_CODE_BISS_IRS_INIT_OK            ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_OK))
#define LOG_CODE_BISS_IRS_POLL_ACK_TIMEOUT   ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_ACK_TIMEOUT))
#define LOG_CODE_BISS_IRS_POLL_FRAME_INVALID ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_FRAME_INVALID))
#define LOG_CODE_BISS_IRS_ZERO_HERE_REJECTED ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_ZERO_HERE_REJECTED))
#endif /* LOGGER_ENABLE_BISS_IRS */

#ifdef LOGGER_ENABLE_LSM6DSX
/* Адресное пространство (старший байт кода), выделенное lsm6dsx (драйвер
 * accel+gyro LSM6DSx/ISM330DHCX). */
#define LOG_ADDR_LSM6DSX 0x86U

/* Смещения (младший байт) - номер события внутри диапазона lsm6dsx. */
#define LOG_OFFSET_LSM6DSX_INIT_OK           0x00U
#define LOG_OFFSET_LSM6DSX_INIT_FAIL_WHOAMI  0x01U
#define LOG_OFFSET_LSM6DSX_BUS_ERROR         0x02U
#define LOG_OFFSET_LSM6DSX_POOL_EXHAUSTED    0x03U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом lsm6dsx. */
#define LOG_CODE_LSM6DSX_INIT_OK          ((uint16_t)((LOG_ADDR_LSM6DSX << 8) | LOG_OFFSET_LSM6DSX_INIT_OK))
#define LOG_CODE_LSM6DSX_INIT_FAIL_WHOAMI ((uint16_t)((LOG_ADDR_LSM6DSX << 8) | LOG_OFFSET_LSM6DSX_INIT_FAIL_WHOAMI))
#define LOG_CODE_LSM6DSX_BUS_ERROR        ((uint16_t)((LOG_ADDR_LSM6DSX << 8) | LOG_OFFSET_LSM6DSX_BUS_ERROR))
#define LOG_CODE_LSM6DSX_POOL_EXHAUSTED   ((uint16_t)((LOG_ADDR_LSM6DSX << 8) | LOG_OFFSET_LSM6DSX_POOL_EXHAUSTED))
#endif /* LOGGER_ENABLE_LSM6DSX */

/* ======================================================================== */
/*  КАТЕГОРИЯ 4/4 - РЕЗЕРВ (0xE0xx-0xFFxx) - пока не используется.          */
/* ======================================================================== */

static const LOGGER_LogEntry_t LOGGER_LogTable[] =
{
    /* ------------------------------------------------------------------ */
    /*  СИСТЕМА / внутренние алгоритмы (0x00xx-0x3Fxx).                    */
    /*  ПРИМЕР - замените на реальные коды логов своего проекта.          */
    /* ------------------------------------------------------------------ */
    { LOG_CODE_SYSTEM_START,     LOGGER_PRIORITY_LOW,  "Система: штатный старт" },
    { LOG_CODE_SYSTEM_HAL_ERROR, LOGGER_PRIORITY_HIGH, "Система: сбой инициализации HAL" },

    /* ------------------------------------------------------------------ */
    /*  ПЕРИФЕРИЯ, интерфейсы МК (0x40xx-0x7Fxx).                          */
    /*  Блок зависимой библиотеки can_manager (CANMGR). Диапазон -         */
    /*  см. LOG_ADDR_CANMGR выше. Включается через                        */
    /*  "#define LOGGER_ENABLE_CANMGR" до #include "logger_codes.h".       */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_CANMGR
    { LOG_CODE_CANMGR_INIT_OK,       LOGGER_PRIORITY_LOW,    "can_manager: Init - шина инициализирована" },
    { LOG_CODE_CANMGR_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "can_manager: Init - ошибка конфига/пула/HAL" },
    { LOG_CODE_CANMGR_REG_REJECTED,  LOGGER_PRIORITY_MEDIUM, "can_manager: фильтр отклонён" },
    { LOG_CODE_CANMGR_TX_QUEUE_FULL, LOGGER_PRIORITY_HIGH,   "can_manager: очередь отправки полна" },
    { LOG_CODE_CANMGR_RX_OVERFLOW,   LOGGER_PRIORITY_MEDIUM, "can_manager: переполнение Rx FIFO0" },
    { LOG_CODE_CANMGR_BUS_OFF,       LOGGER_PRIORITY_HIGH,   "can_manager: Bus-Off, автовосстановлен" },
#endif /* LOGGER_ENABLE_CANMGR */

    /* ------------------------------------------------------------------ */
    /*  ВНЕШНИЕ УСТРОЙСТВА/ДАТЧИКИ на периферии (0x80xx-0xDFxx).           */
    /*  Блок зависимой библиотеки LOAD_PWM (stm32_load_pwm). Диапазон -    */
    /*  см. LOG_ADDR_LOAD_PWM выше. Включается через                      */
    /*  "#define LOGGER_ENABLE_LOAD_PWM" до #include "logger_codes.h".     */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_LOAD_PWM
    { LOG_CODE_LOAD_PWM_INIT_BAD_CONFIG,    LOGGER_PRIORITY_HIGH,   "LOAD_PWM: Init - неверная конфигурация" },
    { LOG_CODE_LOAD_PWM_INIT_POOL_FULL,     LOGGER_PRIORITY_HIGH,   "LOAD_PWM: Init - пул нагрузок исчерпан" },
    { LOG_CODE_LOAD_PWM_INIT_LED_POOL_FULL, LOGGER_PRIORITY_HIGH,   "LOAD_PWM: пул LED-таблиц гаммы исчерпан" },
    { LOG_CODE_LOAD_PWM_INIT_HAL_START_FAIL, LOGGER_PRIORITY_HIGH,  "LOAD_PWM: Init - HAL_TIM_PWM_Start() вернул ошибку" },
    { LOG_CODE_LOAD_PWM_INIT_OK,             LOGGER_PRIORITY_LOW,   "LOAD_PWM: нагрузка зарегистрирована" },
    { LOG_CODE_LOAD_PWM_CYCLE_START,         LOGGER_PRIORITY_LOW,   "LOAD_PWM: Start - запуск цикла" },
    { LOG_CODE_LOAD_PWM_CYCLE_DONE,          LOGGER_PRIORITY_MEDIUM, "LOAD_PWM: Tick - oneshot-цикл завершён" },
    { LOG_CODE_LOAD_PWM_STOP,                LOGGER_PRIORITY_LOW,   "LOAD_PWM: Stop - остановка одной нагрузки" },
    { LOG_CODE_LOAD_PWM_STOP_ALL,            LOGGER_PRIORITY_MEDIUM, "LOAD_PWM: StopAll - массовая остановка" },
    { LOG_CODE_LOAD_PWM_NULL_HANDLE,         LOGGER_PRIORITY_HIGH,  "LOAD_PWM: вызов с h == NULL" },
    { LOG_CODE_LOAD_PWM_GLOBAL_BRIGHTNESS,   LOGGER_PRIORITY_LOW,   "LOAD_PWM: SetGlobalBrightness - смена яркости" },
#endif /* LOGGER_ENABLE_LOAD_PWM */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки rc_bus (rc_bus.c + rc_bus_telemetry.c).  */
    /*  Диапазон - см. LOG_ADDR_RC_BUS выше. Включается через              */
    /*  "#define LOGGER_ENABLE_RC_BUS" до #include "logger_codes.h".       */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_RC_BUS
    { LOG_CODE_RC_BUS_INIT_FAIL,              LOGGER_PRIORITY_HIGH,   "rc_bus: Init - конфигурация отклонена" },
    { LOG_CODE_RC_BUS_UART_ERROR,             LOGGER_PRIORITY_MEDIUM, "rc_bus: ошибка приёма на линии каналов" },
    { LOG_CODE_RC_BUS_FRAME_ERROR,            LOGGER_PRIORITY_LOW,    "rc_bus: битый кадр каналов" },
    { LOG_CODE_RC_BUS_TELEMETRY_INIT_FAIL,    LOGGER_PRIORITY_HIGH,   "rc_bus: TelemetryInit отклонён" },
    { LOG_CODE_RC_BUS_TELEMETRY_UART_ERROR,   LOGGER_PRIORITY_MEDIUM, "rc_bus: ошибка приёма на шине датчиков" },
    { LOG_CODE_RC_BUS_TELEMETRY_FRAME_ERROR,  LOGGER_PRIORITY_LOW,    "rc_bus: битый кадр шины датчиков" },
#endif /* LOGGER_ENABLE_RC_BUS */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки vesc_servo (can_vesc_servo_stm32).       */
    /*  Диапазон - см. LOG_ADDR_VESC_SERVO выше. Включается через          */
    /*  "#define LOGGER_ENABLE_VESC_SERVO" до #include "logger_codes.h".   */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_VESC_SERVO
    { LOG_CODE_VESC_SERVO_INIT_BAD_CONFIG, LOGGER_PRIORITY_HIGH, "vesc_servo: Init - конфигурация отклонена" },
    { LOG_CODE_VESC_SERVO_INIT_POOL_FULL,  LOGGER_PRIORITY_HIGH, "vesc_servo: Init - пул серв исчерпан" },
    { LOG_CODE_VESC_SERVO_INIT_VESC_FAIL,  LOGGER_PRIORITY_HIGH, "vesc_servo: Init - VESC_CAN_Init() вернул ошибку" },
    { LOG_CODE_VESC_SERVO_INIT_DUPLICATE,  LOGGER_PRIORITY_HIGH, "vesc_servo: веска уже занята сервой" },
    { LOG_CODE_VESC_SERVO_INIT_OK,         LOGGER_PRIORITY_LOW,  "vesc_servo: серва зарегистрирована" },
    { LOG_CODE_VESC_SERVO_FAULT_ENTERED,   LOGGER_PRIORITY_HIGH, "vesc_servo: переход в FAULT" },
    { LOG_CODE_VESC_SERVO_HOMING_START,    LOGGER_PRIORITY_LOW,  "vesc_servo: StartHoming - хоуминг запущен" },
    { LOG_CODE_VESC_SERVO_HOMING_DONE,     LOGGER_PRIORITY_LOW,  "vesc_servo: хоуминг успешно завершён" },
#endif /* LOGGER_ENABLE_VESC_SERVO */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки motor_vesc (can_vesc_stm32). Диапазон -  */
    /*  см. LOG_ADDR_VESC выше. Включается через                          */
    /*  "#define LOGGER_ENABLE_VESC" до #include "logger_codes.h".        */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_VESC
    { LOG_CODE_VESC_INIT_OK,       LOGGER_PRIORITY_LOW,    "motor_vesc: Init - веска зарегистрирована" },
    { LOG_CODE_VESC_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "motor_vesc: конфиг/пул/фильтр отклонён" },
    { LOG_CODE_VESC_REG_REJECTED,  LOGGER_PRIORITY_MEDIUM, "motor_vesc: статус отклонён" },
    { LOG_CODE_VESC_EXIST_TIMEOUT, LOGGER_PRIORITY_MEDIUM, "motor_vesc: RequestExists - таймаут ответа PONG" },
#endif /* LOGGER_ENABLE_VESC */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки stm32_biss_irs (энкодер LENZ IRS,        */
    /*  BiSS-C). Диапазон - см. LOG_ADDR_BISS_IRS выше. Включается через   */
    /*  "#define LOGGER_ENABLE_BISS_IRS" до #include "logger_codes.h".     */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_BISS_IRS
    { LOG_CODE_BISS_IRS_INIT_BAD_CONFIG,    LOGGER_PRIORITY_HIGH,   "BISS_IRS: Init - неверная конфигурация" },
    { LOG_CODE_BISS_IRS_INIT_POOL_FULL,     LOGGER_PRIORITY_HIGH,   "BISS_IRS: Init - пул энкодеров исчерпан" },
    { LOG_CODE_BISS_IRS_INIT_CLOCK_RANGE,   LOGGER_PRIORITY_HIGH,   "BISS_IRS: клок недостижим на этом ядре" },
    { LOG_CODE_BISS_IRS_INIT_OK,            LOGGER_PRIORITY_LOW,    "BISS_IRS: Init - энкодер зарегистрирован" },
    { LOG_CODE_BISS_IRS_POLL_ACK_TIMEOUT,   LOGGER_PRIORITY_MEDIUM, "BISS_IRS: Poll - тайм-аут ожидания ACK" },
    { LOG_CODE_BISS_IRS_POLL_FRAME_INVALID, LOGGER_PRIORITY_MEDIUM, "BISS_IRS: кадр невалиден (CRC/ERR/WARN)" },
    { LOG_CODE_BISS_IRS_ZERO_HERE_REJECTED, LOGGER_PRIORITY_LOW,    "BISS_IRS: нет данных для калибровки" },
#endif /* LOGGER_ENABLE_BISS_IRS */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки lsm6dsx (accel+gyro LSM6DSx/ISM330DHCX). */
    /*  Диапазон - см. LOG_ADDR_LSM6DSX выше. Включается через             */
    /*  "#define LOGGER_ENABLE_LSM6DSX" до #include "logger_codes.h".      */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_LSM6DSX
    { LOG_CODE_LSM6DSX_INIT_OK,          LOGGER_PRIORITY_LOW,  "LSM6DSx: инициализирован" },
    { LOG_CODE_LSM6DSX_INIT_FAIL_WHOAMI, LOGGER_PRIORITY_HIGH, "LSM6DSx: WHO_AM_I не совпал" },
    { LOG_CODE_LSM6DSX_BUS_ERROR,        LOGGER_PRIORITY_HIGH, "LSM6DSx: ошибка шины SPI/I2C" },
    { LOG_CODE_LSM6DSX_POOL_EXHAUSTED,   LOGGER_PRIORITY_HIGH, "LSM6DSx: пул экземпляров исчерпан" },
#endif /* LOGGER_ENABLE_LSM6DSX */
};

/** Количество записей в LOGGER_LogTable - используется LOGGER_Init(). */
#define LOGGER_LOG_TABLE_SIZE ((uint32_t)(sizeof(LOGGER_LogTable) / sizeof(LOGGER_LogTable[0])))

#endif /* LOGGER_CODES_H */
