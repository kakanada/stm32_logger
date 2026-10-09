/**
 ******************************************************************************
 * @file    logger_codes.h
 * @brief   Таблица кодов логов проекта - коды, приоритеты, текстовые описания.
 *          Заполняется индивидуально под каждый проект, общая для клиентской
 *          и хостовой сторон библиотеки.
 * @author  Mechanic
 * @date    19.09.2026
 * @version 1.16
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
 * библиотеки (см. logger_types.h).
 *
 * ВАЖНО: описание кода - не длиннее LOGGER_MAX_DESCRIPTION_LENGTH (64) БАЙТ
 * в UTF-8 (кириллица - 2 байта на символ, т.е. ~30 символов). Превышение у
 * ЛЮБОЙ записи таблицы - LOGGER_Init() вернёт HAL_ERROR для всей таблицы. */

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
 *      LOGGER_ENABLE_USB_ETH     - usb_eth                       (0x42, периферия/USB)
 *      LOGGER_ENABLE_USB_DEV     - usb_eth v2.0 (USB + COM)      (0x43, периферия/USB)
 *      LOGGER_ENABLE_LOAD_PWM    - stm32_load_pwm                (0x81, устройство)
 *      LOGGER_ENABLE_RC_BUS      - rc_bus + rc_bus_telemetry      (0x82, устройство)
 *      LOGGER_ENABLE_VESC_SERVO  - can_vesc_servo_stm32           (0x83, устройство)
 *      LOGGER_ENABLE_VESC        - can_vesc_stm32 (motor_vesc)    (0x84, устройство)
 *      LOGGER_ENABLE_BISS_IRS    - stm32_biss_irs                 (0x85, устройство)
 *      LOGGER_ENABLE_LSM6DSX     - lsm6dsx (accel+gyro)           (0x86, устройство)
 *      LOGGER_ENABLE_REG         - stm32_reestr (реестр параметров) (0x03, система)
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
#define LOG_OFFSET_CANMGR_NO_AUTORETRANS 0x06U
#define LOG_OFFSET_CANMGR_TX_INVALID_ARG 0x07U
#define LOG_OFFSET_CANMGR_TX_LEN_CLAMPED 0x08U
#define LOG_OFFSET_CANMGR_TX_HW_FAIL    0x09U
#define LOG_OFFSET_CANMGR_RX_BAD_DLC    0x0AU

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом can_manager. */
#define LOG_CODE_CANMGR_INIT_OK       ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_INIT_OK))
#define LOG_CODE_CANMGR_INIT_FAIL     ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_INIT_FAIL))
#define LOG_CODE_CANMGR_REG_REJECTED  ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_REG_REJECTED))
#define LOG_CODE_CANMGR_TX_QUEUE_FULL ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_TX_QUEUE_FULL))
#define LOG_CODE_CANMGR_RX_OVERFLOW   ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_RX_OVERFLOW))
#define LOG_CODE_CANMGR_BUS_OFF       ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_BUS_OFF))
#define LOG_CODE_CANMGR_NO_AUTORETRANS ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_NO_AUTORETRANS))
#define LOG_CODE_CANMGR_TX_INVALID_ARG ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_TX_INVALID_ARG))
#define LOG_CODE_CANMGR_TX_LEN_CLAMPED ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_TX_LEN_CLAMPED))
#define LOG_CODE_CANMGR_TX_HW_FAIL    ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_TX_HW_FAIL))
#define LOG_CODE_CANMGR_RX_BAD_DLC    ((uint16_t)((LOG_ADDR_CANMGR << 8) | LOG_OFFSET_CANMGR_RX_BAD_DLC))
#endif /* LOGGER_ENABLE_CANMGR */

#ifdef LOGGER_ENABLE_USB_ETH
/* Адресное пространство (старший байт кода), выделенное usb_eth (USB
 * CDC-NCM + lwIP, STM32 как сетевое устройство по USB). */
#define LOG_ADDR_USB_ETH 0x42U

/* Смещения (младший байт) - номер события внутри диапазона usb_eth. */
#define LOG_OFFSET_USB_ETH_INIT_OK       0x00U
#define LOG_OFFSET_USB_ETH_INIT_FAIL     0x01U
#define LOG_OFFSET_USB_ETH_USB_MOUNTED   0x02U
#define LOG_OFFSET_USB_ETH_USB_UNMOUNTED 0x03U
#define LOG_OFFSET_USB_ETH_NET_UP        0x04U
#define LOG_OFFSET_USB_ETH_NET_DOWN      0x05U
#define LOG_OFFSET_USB_ETH_DHCP_TIMEOUT  0x06U
#define LOG_OFFSET_USB_ETH_TCP_ACCEPT    0x07U
#define LOG_OFFSET_USB_ETH_TCP_CLOSED    0x08U
#define LOG_OFFSET_USB_ETH_TCP_POOL_FULL 0x09U
#define LOG_OFFSET_USB_ETH_TCP_ERROR     0x0AU
#define LOG_OFFSET_USB_ETH_RX_DROP       0x0BU
#define LOG_OFFSET_USB_ETH_TX_TIMEOUT    0x0CU
#define LOG_OFFSET_USB_ETH_SEND_NO_MEM   0x0DU
#define LOG_OFFSET_USB_ETH_LISTEN_FAIL   0x0EU
#define LOG_OFFSET_USB_ETH_UDP_BIND_FAIL 0x0FU
#define LOG_OFFSET_USB_ETH_API_ERROR 0x10U
#define LOG_OFFSET_USB_ETH_TX_NO_USB 0x11U
#define LOG_OFFSET_USB_ETH_TCP_SEND_FAIL 0x12U
#define LOG_OFFSET_USB_ETH_UDP_SEND_FAIL 0x13U
#define LOG_OFFSET_USB_ETH_RX_INPUT_FAIL 0x14U
#define LOG_OFFSET_USB_ETH_LWIP_MEM_ERR 0x15U
#define LOG_OFFSET_USB_ETH_LWIP_ASSERT 0x16U
#define LOG_OFFSET_USB_ETH_LWIP_ARG_ERR 0x17U
#define LOG_OFFSET_USB_ETH_TCP_CLOSE_RST 0x18U
#define LOG_OFFSET_USB_ETH_TCP_ACCEPT_ERR 0x19U
#define LOG_OFFSET_USB_ETH_UDP_RX_TRUNC 0x1AU

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом usb_eth. */
#define LOG_CODE_USB_ETH_INIT_OK       ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_INIT_OK))
#define LOG_CODE_USB_ETH_INIT_FAIL     ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_INIT_FAIL))
#define LOG_CODE_USB_ETH_USB_MOUNTED   ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_USB_MOUNTED))
#define LOG_CODE_USB_ETH_USB_UNMOUNTED ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_USB_UNMOUNTED))
#define LOG_CODE_USB_ETH_NET_UP        ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_NET_UP))
#define LOG_CODE_USB_ETH_NET_DOWN      ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_NET_DOWN))
#define LOG_CODE_USB_ETH_DHCP_TIMEOUT  ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_DHCP_TIMEOUT))
#define LOG_CODE_USB_ETH_TCP_ACCEPT    ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_ACCEPT))
#define LOG_CODE_USB_ETH_TCP_CLOSED    ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_CLOSED))
#define LOG_CODE_USB_ETH_TCP_POOL_FULL ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_POOL_FULL))
#define LOG_CODE_USB_ETH_TCP_ERROR     ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_ERROR))
#define LOG_CODE_USB_ETH_RX_DROP       ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_RX_DROP))
#define LOG_CODE_USB_ETH_TX_TIMEOUT    ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TX_TIMEOUT))
#define LOG_CODE_USB_ETH_SEND_NO_MEM   ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_SEND_NO_MEM))
#define LOG_CODE_USB_ETH_LISTEN_FAIL   ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_LISTEN_FAIL))
#define LOG_CODE_USB_ETH_UDP_BIND_FAIL ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_UDP_BIND_FAIL))
#define LOG_CODE_USB_ETH_API_ERROR ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_API_ERROR))
#define LOG_CODE_USB_ETH_TX_NO_USB ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TX_NO_USB))
#define LOG_CODE_USB_ETH_TCP_SEND_FAIL ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_SEND_FAIL))
#define LOG_CODE_USB_ETH_UDP_SEND_FAIL ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_UDP_SEND_FAIL))
#define LOG_CODE_USB_ETH_RX_INPUT_FAIL ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_RX_INPUT_FAIL))
#define LOG_CODE_USB_ETH_LWIP_MEM_ERR ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_LWIP_MEM_ERR))
#define LOG_CODE_USB_ETH_LWIP_ASSERT ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_LWIP_ASSERT))
#define LOG_CODE_USB_ETH_LWIP_ARG_ERR ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_LWIP_ARG_ERR))
#define LOG_CODE_USB_ETH_TCP_CLOSE_RST ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_CLOSE_RST))
#define LOG_CODE_USB_ETH_TCP_ACCEPT_ERR ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_TCP_ACCEPT_ERR))
#define LOG_CODE_USB_ETH_UDP_RX_TRUNC ((uint16_t)((LOG_ADDR_USB_ETH << 8) | LOG_OFFSET_USB_ETH_UDP_RX_TRUNC))
#endif /* LOGGER_ENABLE_USB_ETH */

#ifdef LOGGER_ENABLE_USB_DEV
/* Адресное пространство (старший байт кода), выделенное usb_eth v2.0
 * (общая часть USB + виртуальный COM-порт). Независимо от 0x42. */
#define LOG_ADDR_USB_DEV 0x43U

/* Смещения (младший байт) - номер события внутри диапазона usb_dev. */
#define LOG_OFFSET_USB_DEV_USB_START_FAIL    0x00U
#define LOG_OFFSET_USB_DEV_INIT_REFUSED      0x01U
#define LOG_OFFSET_USB_DEV_USB_CONNECTED     0x02U
#define LOG_OFFSET_USB_DEV_USB_DISCONNECTED  0x03U
#define LOG_OFFSET_USB_DEV_REENUM            0x04U
#define LOG_OFFSET_USB_DEV_COM_INIT          0x05U
#define LOG_OFFSET_USB_DEV_COM_DEINIT        0x06U
#define LOG_OFFSET_USB_DEV_COM_OPEN          0x07U
#define LOG_OFFSET_USB_DEV_COM_CLOSE         0x08U
#define LOG_OFFSET_USB_DEV_COM_TX_BUSY       0x09U
#define LOG_OFFSET_USB_DEV_ETH_DEINIT        0x0AU
#define LOG_OFFSET_USB_DEV_API_ERROR 0x0BU
#define LOG_OFFSET_USB_DEV_COM_TX_NOT_READY 0x0CU
#define LOG_OFFSET_USB_DEV_COM_TX_DISCARD 0x0DU
#define LOG_OFFSET_USB_DEV_USB_CONNECT_FAIL 0x0EU
#define LOG_OFFSET_USB_DEV_TUSB_ASSERT 0x0FU
#define LOG_OFFSET_USB_DEV_LOG_SUPPRESSED 0x10U

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом usb_dev. */
#define LOG_CODE_USB_DEV_USB_START_FAIL    ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_USB_START_FAIL))
#define LOG_CODE_USB_DEV_INIT_REFUSED      ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_INIT_REFUSED))
#define LOG_CODE_USB_DEV_USB_CONNECTED     ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_USB_CONNECTED))
#define LOG_CODE_USB_DEV_USB_DISCONNECTED  ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_USB_DISCONNECTED))
#define LOG_CODE_USB_DEV_REENUM            ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_REENUM))
#define LOG_CODE_USB_DEV_COM_INIT          ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_INIT))
#define LOG_CODE_USB_DEV_COM_DEINIT        ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_DEINIT))
#define LOG_CODE_USB_DEV_COM_OPEN          ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_OPEN))
#define LOG_CODE_USB_DEV_COM_CLOSE         ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_CLOSE))
#define LOG_CODE_USB_DEV_COM_TX_BUSY       ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_TX_BUSY))
#define LOG_CODE_USB_DEV_ETH_DEINIT        ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_ETH_DEINIT))
#define LOG_CODE_USB_DEV_API_ERROR ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_API_ERROR))
#define LOG_CODE_USB_DEV_COM_TX_NOT_READY ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_TX_NOT_READY))
#define LOG_CODE_USB_DEV_COM_TX_DISCARD ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_COM_TX_DISCARD))
#define LOG_CODE_USB_DEV_USB_CONNECT_FAIL ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_USB_CONNECT_FAIL))
#define LOG_CODE_USB_DEV_TUSB_ASSERT ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_TUSB_ASSERT))
#define LOG_CODE_USB_DEV_LOG_SUPPRESSED ((uint16_t)((LOG_ADDR_USB_DEV << 8) | LOG_OFFSET_USB_DEV_LOG_SUPPRESSED))
#endif /* LOGGER_ENABLE_USB_DEV */

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
#define LOG_OFFSET_VESC_EXIST_OK           0x04U
#define LOG_OFFSET_VESC_CTRL_MODE          0x05U
#define LOG_OFFSET_VESC_SEND_FAIL          0x06U
#define LOG_OFFSET_VESC_RX_BAD_LEN         0x07U
#define LOG_OFFSET_VESC_BAD_VALUE          0x08U
#define LOG_OFFSET_VESC_MISUSE             0x09U
#define LOG_OFFSET_VESC_POSMEM             0x0AU
#define LOG_OFFSET_VESC_LOG_SUPPRESSED     0x0BU
#define LOG_OFFSET_VESC_BR_INIT_OK         0x10U
#define LOG_OFFSET_VESC_BR_INIT_FAIL       0x11U
#define LOG_OFFSET_VESC_BR_MCCONF_READ     0x12U
#define LOG_OFFSET_VESC_BR_MCCONF_WRITE    0x13U
#define LOG_OFFSET_VESC_BR_APPCONF_READ    0x14U
#define LOG_OFFSET_VESC_BR_APPCONF_WRITE   0x15U
#define LOG_OFFSET_VESC_BR_FW_BOOTLOADER   0x16U
#define LOG_OFFSET_VESC_BR_FW_ERASE        0x17U
#define LOG_OFFSET_VESC_BR_FW_WRITE        0x18U
#define LOG_OFFSET_VESC_BR_FW_ERROR        0x19U
#define LOG_OFFSET_VESC_BR_SCAN_START      0x1AU
#define LOG_OFFSET_VESC_BR_SCAN_FOUND      0x1BU
#define LOG_OFFSET_VESC_BR_SCAN_DONE       0x1CU
#define LOG_OFFSET_VESC_BR_REPLY_TIMEOUT   0x1DU
#define LOG_OFFSET_VESC_BR_QUEUE_OVERFLOW  0x1EU
#define LOG_OFFSET_VESC_BR_RX_ERROR        0x1FU
#define LOG_OFFSET_VESC_BR_RX_TIMEOUT      0x20U
#define LOG_OFFSET_VESC_BR_CAN_CRC_ERROR   0x21U
#define LOG_OFFSET_VESC_BR_FAULT           0x22U
#define LOG_OFFSET_VESC_BR_INIT_BAD_CONFIG 0x23U
#define LOG_OFFSET_VESC_BR_INIT_POOL_FULL  0x24U
#define LOG_OFFSET_VESC_BR_INIT_FILTER_FAIL 0x25U
#define LOG_OFFSET_VESC_BR_FWD_TOO_BIG     0x26U
#define LOG_OFFSET_VESC_BR_FWD_SEND_BUSY   0x27U
#define LOG_OFFSET_VESC_BR_FWD_SEND_ERROR  0x28U
#define LOG_OFFSET_VESC_BR_RX_CRC_ERROR    0x29U
#define LOG_OFFSET_VESC_BR_RX_BAD_LEN      0x2AU
#define LOG_OFFSET_VESC_BR_CAN_BAD_FRAME   0x2BU
#define LOG_OFFSET_VESC_BR_CAN_FILL_OVERFLOW 0x2CU
#define LOG_OFFSET_VESC_BR_CAN_LEN_ERROR   0x2DU
#define LOG_OFFSET_VESC_BR_FWD_BAD_FRAME   0x2EU
#define LOG_OFFSET_VESC_BR_TX_TOO_BIG      0x2FU

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом motor_vesc. */
#define LOG_CODE_VESC_INIT_OK       ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_INIT_OK))
#define LOG_CODE_VESC_INIT_FAIL     ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_INIT_FAIL))
#define LOG_CODE_VESC_REG_REJECTED  ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_REG_REJECTED))
#define LOG_CODE_VESC_EXIST_TIMEOUT ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_EXIST_TIMEOUT))
#define LOG_CODE_VESC_EXIST_OK           ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_EXIST_OK))
#define LOG_CODE_VESC_CTRL_MODE          ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_CTRL_MODE))
#define LOG_CODE_VESC_SEND_FAIL          ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_SEND_FAIL))
#define LOG_CODE_VESC_RX_BAD_LEN         ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_RX_BAD_LEN))
#define LOG_CODE_VESC_BAD_VALUE          ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BAD_VALUE))
#define LOG_CODE_VESC_MISUSE             ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_MISUSE))
#define LOG_CODE_VESC_POSMEM             ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_POSMEM))
#define LOG_CODE_VESC_LOG_SUPPRESSED     ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_LOG_SUPPRESSED))
#define LOG_CODE_VESC_BR_INIT_OK         ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_INIT_OK))
#define LOG_CODE_VESC_BR_INIT_FAIL       ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_INIT_FAIL))
#define LOG_CODE_VESC_BR_MCCONF_READ     ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_MCCONF_READ))
#define LOG_CODE_VESC_BR_MCCONF_WRITE    ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_MCCONF_WRITE))
#define LOG_CODE_VESC_BR_APPCONF_READ    ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_APPCONF_READ))
#define LOG_CODE_VESC_BR_APPCONF_WRITE   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_APPCONF_WRITE))
#define LOG_CODE_VESC_BR_FW_BOOTLOADER   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FW_BOOTLOADER))
#define LOG_CODE_VESC_BR_FW_ERASE        ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FW_ERASE))
#define LOG_CODE_VESC_BR_FW_WRITE        ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FW_WRITE))
#define LOG_CODE_VESC_BR_FW_ERROR        ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FW_ERROR))
#define LOG_CODE_VESC_BR_SCAN_START      ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_SCAN_START))
#define LOG_CODE_VESC_BR_SCAN_FOUND      ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_SCAN_FOUND))
#define LOG_CODE_VESC_BR_SCAN_DONE       ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_SCAN_DONE))
#define LOG_CODE_VESC_BR_REPLY_TIMEOUT   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_REPLY_TIMEOUT))
#define LOG_CODE_VESC_BR_QUEUE_OVERFLOW  ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_QUEUE_OVERFLOW))
#define LOG_CODE_VESC_BR_RX_ERROR        ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_RX_ERROR))
#define LOG_CODE_VESC_BR_RX_TIMEOUT      ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_RX_TIMEOUT))
#define LOG_CODE_VESC_BR_CAN_CRC_ERROR   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_CAN_CRC_ERROR))
#define LOG_CODE_VESC_BR_FAULT           ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FAULT))
#define LOG_CODE_VESC_BR_INIT_BAD_CONFIG ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_INIT_BAD_CONFIG))
#define LOG_CODE_VESC_BR_INIT_POOL_FULL  ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_INIT_POOL_FULL))
#define LOG_CODE_VESC_BR_INIT_FILTER_FAIL ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_INIT_FILTER_FAIL))
#define LOG_CODE_VESC_BR_FWD_TOO_BIG     ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FWD_TOO_BIG))
#define LOG_CODE_VESC_BR_FWD_SEND_BUSY   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FWD_SEND_BUSY))
#define LOG_CODE_VESC_BR_FWD_SEND_ERROR  ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FWD_SEND_ERROR))
#define LOG_CODE_VESC_BR_RX_CRC_ERROR    ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_RX_CRC_ERROR))
#define LOG_CODE_VESC_BR_RX_BAD_LEN      ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_RX_BAD_LEN))
#define LOG_CODE_VESC_BR_CAN_BAD_FRAME   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_CAN_BAD_FRAME))
#define LOG_CODE_VESC_BR_CAN_FILL_OVERFLOW ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_CAN_FILL_OVERFLOW))
#define LOG_CODE_VESC_BR_CAN_LEN_ERROR   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_CAN_LEN_ERROR))
#define LOG_CODE_VESC_BR_FWD_BAD_FRAME   ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_FWD_BAD_FRAME))
#define LOG_CODE_VESC_BR_TX_TOO_BIG      ((uint16_t)((LOG_ADDR_VESC << 8) | LOG_OFFSET_VESC_BR_TX_TOO_BIG))
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
#define LOG_OFFSET_BISS_IRS_POLL_BAD_HANDLE    0x07U
#define LOG_OFFSET_BISS_IRS_POLL_ENC_ERROR     0x08U
#define LOG_OFFSET_BISS_IRS_POLL_ENC_WARNING   0x09U
#define LOG_OFFSET_BISS_IRS_POLL_CRC_ERROR     0x0AU
#define LOG_OFFSET_BISS_IRS_POLL_FRAMING_ERROR 0x0BU
#define LOG_OFFSET_BISS_IRS_LINK_RECOVERED     0x0CU
#define LOG_OFFSET_BISS_IRS_SET_BAD_HANDLE     0x0DU

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом stm32_biss_irs. */
#define LOG_CODE_BISS_IRS_INIT_BAD_CONFIG    ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_BAD_CONFIG))
#define LOG_CODE_BISS_IRS_INIT_POOL_FULL     ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_POOL_FULL))
#define LOG_CODE_BISS_IRS_INIT_CLOCK_RANGE   ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_CLOCK_RANGE))
#define LOG_CODE_BISS_IRS_INIT_OK            ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_INIT_OK))
#define LOG_CODE_BISS_IRS_POLL_ACK_TIMEOUT   ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_ACK_TIMEOUT))
#define LOG_CODE_BISS_IRS_POLL_FRAME_INVALID ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_FRAME_INVALID))
#define LOG_CODE_BISS_IRS_ZERO_HERE_REJECTED ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_ZERO_HERE_REJECTED))
#define LOG_CODE_BISS_IRS_POLL_BAD_HANDLE    ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_BAD_HANDLE))
#define LOG_CODE_BISS_IRS_POLL_ENC_ERROR     ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_ENC_ERROR))
#define LOG_CODE_BISS_IRS_POLL_ENC_WARNING   ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_ENC_WARNING))
#define LOG_CODE_BISS_IRS_POLL_CRC_ERROR     ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_CRC_ERROR))
#define LOG_CODE_BISS_IRS_POLL_FRAMING_ERROR ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_POLL_FRAMING_ERROR))
#define LOG_CODE_BISS_IRS_LINK_RECOVERED     ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_LINK_RECOVERED))
#define LOG_CODE_BISS_IRS_SET_BAD_HANDLE     ((uint16_t)((LOG_ADDR_BISS_IRS << 8) | LOG_OFFSET_BISS_IRS_SET_BAD_HANDLE))
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

#ifdef LOGGER_ENABLE_REG
/* Адресное пространство (старший байт кода), выделенное stm32_reestr
 * (реестр параметров с NVM и протоколом хоста). Категория СИСТЕМА. */
#define LOG_ADDR_REG 0x03U

/* Смещения (младший байт) - номер события внутри диапазона реестра. */
#define LOG_OFFSET_REG_INIT          0x01U
#define LOG_OFFSET_REG_INIT_FAIL     0x02U
#define LOG_OFFSET_REG_HOST_WRITE    0x03U
#define LOG_OFFSET_REG_HOST_REJECT   0x04U
#define LOG_OFFSET_REG_HOST_HELLO    0x05U
#define LOG_OFFSET_REG_BAD_FRAME     0x06U
#define LOG_OFFSET_REG_NVM_LOADED    0x07U
#define LOG_OFFSET_REG_NVM_DEFAULTED 0x08U
#define LOG_OFFSET_REG_NVM_SAVED     0x09U
#define LOG_OFFSET_REG_NVM_ERROR     0x0AU

/* Итоговые коды - используются и в LOGGER_LogTable ниже, и в самом реестре. */
#define LOG_CODE_REG_INIT          ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_INIT))
#define LOG_CODE_REG_INIT_FAIL     ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_INIT_FAIL))
#define LOG_CODE_REG_HOST_WRITE    ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_HOST_WRITE))
#define LOG_CODE_REG_HOST_REJECT   ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_HOST_REJECT))
#define LOG_CODE_REG_HOST_HELLO    ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_HOST_HELLO))
#define LOG_CODE_REG_BAD_FRAME     ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_BAD_FRAME))
#define LOG_CODE_REG_NVM_LOADED    ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_NVM_LOADED))
#define LOG_CODE_REG_NVM_DEFAULTED ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_NVM_DEFAULTED))
#define LOG_CODE_REG_NVM_SAVED     ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_NVM_SAVED))
#define LOG_CODE_REG_NVM_ERROR     ((uint16_t)((LOG_ADDR_REG << 8) | LOG_OFFSET_REG_NVM_ERROR))
#endif /* LOGGER_ENABLE_REG */

/* ======================================================================== */
/*  КАТЕГОРИЯ 4/4 - РЕЗЕРВ (0xE0xx-0xFFxx) - пока не используется.          */
/* ======================================================================== */

static const LOGGER_LogEntry_t LOGGER_LogTable[] =
{
    /* ------------------------------------------------------------------ */
    /*  СИСТЕМА / внутренние алгоритмы (0x00xx-0x3Fxx).                    */
    /*  ПРИМЕР - замените на реальные коды логов своего проекта.          */
    /* ------------------------------------------------------------------ */
    { LOG_CODE_SYSTEM_START,     LOGGER_PRIORITY_LOW,  "штатный старт" },
    { LOG_CODE_SYSTEM_HAL_ERROR, LOGGER_PRIORITY_HIGH, "сбой инициализации HAL" },
#ifdef LOGGER_ENABLE_REG
    { LOG_CODE_REG_INIT,          LOGGER_PRIORITY_LOW,    "Реестр инициализирован" },
    { LOG_CODE_REG_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "Ошибка инициализации реестра" },
    { LOG_CODE_REG_HOST_WRITE,    LOGGER_PRIORITY_LOW,    "Хост изменил параметр" },
    { LOG_CODE_REG_HOST_REJECT,   LOGGER_PRIORITY_MEDIUM, "Запись хоста отклонена" },
    { LOG_CODE_REG_HOST_HELLO,    LOGGER_PRIORITY_LOW,    "Хост подключился" },
    { LOG_CODE_REG_BAD_FRAME,     LOGGER_PRIORITY_MEDIUM, "Битый кадр от хоста" },
    { LOG_CODE_REG_NVM_LOADED,    LOGGER_PRIORITY_LOW,    "Папка загружена из NVM" },
    { LOG_CODE_REG_NVM_DEFAULTED, LOGGER_PRIORITY_MEDIUM, "Папка сброшена в умолчания" },
    { LOG_CODE_REG_NVM_SAVED,     LOGGER_PRIORITY_LOW,    "Папка сохранена в NVM" },
    { LOG_CODE_REG_NVM_ERROR,     LOGGER_PRIORITY_HIGH,   "Ошибка записи реестра в NVM" },
#endif /* LOGGER_ENABLE_REG */

    /* ------------------------------------------------------------------ */
    /*  ПЕРИФЕРИЯ, интерфейсы МК (0x40xx-0x7Fxx).                          */
    /*  Блок зависимой библиотеки can_manager (CANMGR). Диапазон -         */
    /*  см. LOG_ADDR_CANMGR выше. Включается через                        */
    /*  "#define LOGGER_ENABLE_CANMGR" до #include "logger_codes.h".       */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_CANMGR
    { LOG_CODE_CANMGR_INIT_OK,       LOGGER_PRIORITY_LOW,    "Init - шина инициализирована" },
    { LOG_CODE_CANMGR_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "Init - ошибка конфига/пула/HAL" },
    { LOG_CODE_CANMGR_REG_REJECTED,  LOGGER_PRIORITY_MEDIUM, "фильтр отклонён" },
    { LOG_CODE_CANMGR_TX_QUEUE_FULL, LOGGER_PRIORITY_HIGH,   "очередь отправки полна" },
    { LOG_CODE_CANMGR_RX_OVERFLOW,   LOGGER_PRIORITY_MEDIUM, "переполнение Rx FIFO0" },
    { LOG_CODE_CANMGR_BUS_OFF,       LOGGER_PRIORITY_HIGH,   "Bus-Off, автовосстановлен" },
    { LOG_CODE_CANMGR_NO_AUTORETRANS, LOGGER_PRIORITY_MEDIUM, "AutoRetransmission отключен (DAR/NART)" },
    { LOG_CODE_CANMGR_TX_INVALID_ARG, LOGGER_PRIORITY_MEDIUM, "Send: bus/data == NULL (value=id кадра)" },
    { LOG_CODE_CANMGR_TX_LEN_CLAMPED, LOGGER_PRIORITY_LOW,    "Send: len>8 обрезан (value=len)" },
    { LOG_CODE_CANMGR_TX_HW_FAIL,     LOGGER_PRIORITY_MEDIUM, "отказ HAL при Tx, кадр в очереди (id)" },
    { LOG_CODE_CANMGR_RX_BAD_DLC,     LOGGER_PRIORITY_LOW,    "Rx: DLC>8, обрезан до 8 (value=DLC)" },
#endif /* LOGGER_ENABLE_CANMGR */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки usb_eth (USB CDC-NCM + lwIP). Диапазон - */
    /*  см. LOG_ADDR_USB_ETH выше. Включается через                       */
    /*  "#define LOGGER_ENABLE_USB_ETH" до #include "logger_codes.h".     */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_USB_ETH
    { LOG_CODE_USB_ETH_INIT_OK,       LOGGER_PRIORITY_LOW,    "инициализация выполнена" },
    { LOG_CODE_USB_ETH_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "ошибка инициализации" },
    { LOG_CODE_USB_ETH_USB_MOUNTED,   LOGGER_PRIORITY_LOW,    "USB подключён к хосту" },
    { LOG_CODE_USB_ETH_USB_UNMOUNTED, LOGGER_PRIORITY_MEDIUM, "USB отключён от хоста" },
    { LOG_CODE_USB_ETH_NET_UP,        LOGGER_PRIORITY_LOW,    "сеть готова, IP назначен" },
    { LOG_CODE_USB_ETH_NET_DOWN,      LOGGER_PRIORITY_MEDIUM, "сеть потеряна" },
    { LOG_CODE_USB_ETH_DHCP_TIMEOUT,  LOGGER_PRIORITY_MEDIUM, "DHCP-клиент не получил адрес" },
    { LOG_CODE_USB_ETH_TCP_ACCEPT,    LOGGER_PRIORITY_LOW,    "TCP-клиент подключился" },
    { LOG_CODE_USB_ETH_TCP_CLOSED,    LOGGER_PRIORITY_LOW,    "TCP-соединение закрыто" },
    { LOG_CODE_USB_ETH_TCP_POOL_FULL, LOGGER_PRIORITY_HIGH,   "нет свободных слотов TCP, отказ" },
    { LOG_CODE_USB_ETH_TCP_ERROR,     LOGGER_PRIORITY_MEDIUM, "TCP-соединение сброшено/ошибка" },
    { LOG_CODE_USB_ETH_RX_DROP,       LOGGER_PRIORITY_MEDIUM, "RX кадр отброшен (нет памяти)" },
    { LOG_CODE_USB_ETH_TX_TIMEOUT,    LOGGER_PRIORITY_MEDIUM, "таймаут TX, кадр отброшен" },
    { LOG_CODE_USB_ETH_SEND_NO_MEM,   LOGGER_PRIORITY_MEDIUM, "нет памяти lwIP для отправки" },
    { LOG_CODE_USB_ETH_LISTEN_FAIL,   LOGGER_PRIORITY_HIGH,   "не удалось открыть TCP-порт" },
    { LOG_CODE_USB_ETH_UDP_BIND_FAIL, LOGGER_PRIORITY_HIGH,   "не удалось открыть UDP-порт" },
    { LOG_CODE_USB_ETH_API_ERROR, LOGGER_PRIORITY_MEDIUM, "неверный вызов API сети" },
    { LOG_CODE_USB_ETH_TX_NO_USB, LOGGER_PRIORITY_MEDIUM, "кадр не отправлен: USB не подключён" },
    { LOG_CODE_USB_ETH_TCP_SEND_FAIL, LOGGER_PRIORITY_MEDIUM, "ошибка отправки TCP (lwIP)" },
    { LOG_CODE_USB_ETH_UDP_SEND_FAIL, LOGGER_PRIORITY_MEDIUM, "ошибка отправки UDP (lwIP)" },
    { LOG_CODE_USB_ETH_RX_INPUT_FAIL, LOGGER_PRIORITY_MEDIUM, "lwIP отверг входящий кадр" },
    { LOG_CODE_USB_ETH_LWIP_MEM_ERR, LOGGER_PRIORITY_HIGH, "нехватка памяти в lwIP" },
    { LOG_CODE_USB_ETH_LWIP_ASSERT, LOGGER_PRIORITY_HIGH, "внутренняя проверка lwIP" },
    { LOG_CODE_USB_ETH_LWIP_ARG_ERR, LOGGER_PRIORITY_HIGH, "неверные аргументы функции lwIP" },
    { LOG_CODE_USB_ETH_TCP_CLOSE_RST, LOGGER_PRIORITY_MEDIUM, "нет памяти на FIN, закрыто сбросом" },
    { LOG_CODE_USB_ETH_TCP_ACCEPT_ERR, LOGGER_PRIORITY_MEDIUM, "ошибка входящего TCP-подключения" },
    { LOG_CODE_USB_ETH_UDP_RX_TRUNC, LOGGER_PRIORITY_MEDIUM, "входящая датаграмма UDP обрезана" },
#endif /* LOGGER_ENABLE_USB_ETH */

    /* ------------------------------------------------------------------ */
    /*  Блок usb_eth v2.0 (usb_dev: общая часть USB + виртуальный COM).    */
    /*  Диапазон - см. LOG_ADDR_USB_DEV выше. Включается через            */
    /*  "#define LOGGER_ENABLE_USB_DEV" до #include "logger_codes.h".      */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_USB_DEV
    { LOG_CODE_USB_DEV_USB_START_FAIL,   LOGGER_PRIORITY_HIGH,   "TinyUSB не запустился" },
    { LOG_CODE_USB_DEV_INIT_REFUSED,     LOGGER_PRIORITY_HIGH,   "Init отклонён - нет точек сети+COM" },
    { LOG_CODE_USB_DEV_USB_CONNECTED,    LOGGER_PRIORITY_LOW,    "подключено к ПК" },
    { LOG_CODE_USB_DEV_USB_DISCONNECTED, LOGGER_PRIORITY_MEDIUM, "отключено от ПК (кабель/сон ПК)" },
    { LOG_CODE_USB_DEV_REENUM,           LOGGER_PRIORITY_LOW,    "смена набора устройств (re-enum)" },
    { LOG_CODE_USB_DEV_COM_INIT,         LOGGER_PRIORITY_LOW,    "COM: включён" },
    { LOG_CODE_USB_DEV_COM_DEINIT,       LOGGER_PRIORITY_LOW,    "COM: выключен" },
    { LOG_CODE_USB_DEV_COM_OPEN,         LOGGER_PRIORITY_LOW,    "COM: порт открыт на ПК (DTR)" },
    { LOG_CODE_USB_DEV_COM_CLOSE,        LOGGER_PRIORITY_LOW,    "COM: порт закрыт на ПК" },
    { LOG_CODE_USB_DEV_COM_TX_BUSY,      LOGGER_PRIORITY_MEDIUM, "COM: TX отклонён - буфер полон" },
    { LOG_CODE_USB_DEV_ETH_DEINIT,       LOGGER_PRIORITY_LOW,    "ETH: сеть выключена (DeInit)" },
    { LOG_CODE_USB_DEV_API_ERROR, LOGGER_PRIORITY_MEDIUM, "неверный вызов API USB/COM" },
    { LOG_CODE_USB_DEV_COM_TX_NOT_READY, LOGGER_PRIORITY_MEDIUM, "COM TX: не подключён к ПК" },
    { LOG_CODE_USB_DEV_COM_TX_DISCARD, LOGGER_PRIORITY_MEDIUM, "COM: неотправленные данные стёрты" },
    { LOG_CODE_USB_DEV_USB_CONNECT_FAIL, LOGGER_PRIORITY_HIGH, "TinyUSB не подключил/отключил USB" },
    { LOG_CODE_USB_DEV_TUSB_ASSERT, LOGGER_PRIORITY_HIGH, "внутренняя ошибка TinyUSB" },
    { LOG_CODE_USB_DEV_LOG_SUPPRESSED, LOGGER_PRIORITY_MEDIUM, "записи подавлены (частота/повтор)" },
#endif /* LOGGER_ENABLE_USB_DEV */

    /* ------------------------------------------------------------------ */
    /*  ВНЕШНИЕ УСТРОЙСТВА/ДАТЧИКИ на периферии (0x80xx-0xDFxx).           */
    /*  Блок зависимой библиотеки LOAD_PWM (stm32_load_pwm). Диапазон -    */
    /*  см. LOG_ADDR_LOAD_PWM выше. Включается через                      */
    /*  "#define LOGGER_ENABLE_LOAD_PWM" до #include "logger_codes.h".     */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_LOAD_PWM
    { LOG_CODE_LOAD_PWM_INIT_BAD_CONFIG,    LOGGER_PRIORITY_HIGH,   "Init - неверная конфигурация" },
    { LOG_CODE_LOAD_PWM_INIT_POOL_FULL,     LOGGER_PRIORITY_HIGH,   "Init - пул нагрузок исчерпан" },
    { LOG_CODE_LOAD_PWM_INIT_LED_POOL_FULL, LOGGER_PRIORITY_HIGH,   "пул LED-таблиц гаммы исчерпан" },
    { LOG_CODE_LOAD_PWM_INIT_HAL_START_FAIL, LOGGER_PRIORITY_HIGH,  "Init - HAL_TIM_PWM_Start() вернул ошибку" },
    { LOG_CODE_LOAD_PWM_INIT_OK,             LOGGER_PRIORITY_LOW,   "нагрузка зарегистрирована" },
    { LOG_CODE_LOAD_PWM_CYCLE_START,         LOGGER_PRIORITY_LOW,   "Start - запуск цикла" },
    { LOG_CODE_LOAD_PWM_CYCLE_DONE,          LOGGER_PRIORITY_MEDIUM, "Tick - oneshot-цикл завершён" },
    { LOG_CODE_LOAD_PWM_STOP,                LOGGER_PRIORITY_LOW,   "Stop - остановка одной нагрузки" },
    { LOG_CODE_LOAD_PWM_STOP_ALL,            LOGGER_PRIORITY_MEDIUM, "StopAll - массовая остановка" },
    { LOG_CODE_LOAD_PWM_NULL_HANDLE,         LOGGER_PRIORITY_HIGH,  "вызов с h == NULL" },
    { LOG_CODE_LOAD_PWM_GLOBAL_BRIGHTNESS,   LOGGER_PRIORITY_LOW,   "SetGlobalBrightness - смена яркости" },
#endif /* LOGGER_ENABLE_LOAD_PWM */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки rc_bus (rc_bus.c + rc_bus_telemetry.c).  */
    /*  Диапазон - см. LOG_ADDR_RC_BUS выше. Включается через              */
    /*  "#define LOGGER_ENABLE_RC_BUS" до #include "logger_codes.h".       */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_RC_BUS
    { LOG_CODE_RC_BUS_INIT_FAIL,              LOGGER_PRIORITY_HIGH,   "Init - конфигурация отклонена" },
    { LOG_CODE_RC_BUS_UART_ERROR,             LOGGER_PRIORITY_MEDIUM, "ошибка приёма на линии каналов" },
    { LOG_CODE_RC_BUS_FRAME_ERROR,            LOGGER_PRIORITY_LOW,    "битый кадр каналов" },
    { LOG_CODE_RC_BUS_TELEMETRY_INIT_FAIL,    LOGGER_PRIORITY_HIGH,   "TelemetryInit отклонён" },
    { LOG_CODE_RC_BUS_TELEMETRY_UART_ERROR,   LOGGER_PRIORITY_MEDIUM, "ошибка приёма на шине датчиков" },
    { LOG_CODE_RC_BUS_TELEMETRY_FRAME_ERROR,  LOGGER_PRIORITY_LOW,    "битый кадр шины датчиков" },
#endif /* LOGGER_ENABLE_RC_BUS */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки vesc_servo (can_vesc_servo_stm32).       */
    /*  Диапазон - см. LOG_ADDR_VESC_SERVO выше. Включается через          */
    /*  "#define LOGGER_ENABLE_VESC_SERVO" до #include "logger_codes.h".   */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_VESC_SERVO
    { LOG_CODE_VESC_SERVO_INIT_BAD_CONFIG, LOGGER_PRIORITY_HIGH, "Init - конфигурация отклонена" },
    { LOG_CODE_VESC_SERVO_INIT_POOL_FULL,  LOGGER_PRIORITY_HIGH, "Init - пул серв исчерпан" },
    { LOG_CODE_VESC_SERVO_INIT_VESC_FAIL,  LOGGER_PRIORITY_HIGH, "Init - VESC_CAN_Init() вернул ошибку" },
    { LOG_CODE_VESC_SERVO_INIT_DUPLICATE,  LOGGER_PRIORITY_HIGH, "веска уже занята сервой" },
    { LOG_CODE_VESC_SERVO_INIT_OK,         LOGGER_PRIORITY_LOW,  "серва зарегистрирована" },
    { LOG_CODE_VESC_SERVO_FAULT_ENTERED,   LOGGER_PRIORITY_HIGH, "переход в FAULT" },
    { LOG_CODE_VESC_SERVO_HOMING_START,    LOGGER_PRIORITY_LOW,  "StartHoming - хоуминг запущен" },
    { LOG_CODE_VESC_SERVO_HOMING_DONE,     LOGGER_PRIORITY_LOW,  "хоуминг успешно завершён" },
#endif /* LOGGER_ENABLE_VESC_SERVO */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки motor_vesc (can_vesc_stm32). Диапазон -  */
    /*  см. LOG_ADDR_VESC выше. Включается через                          */
    /*  "#define LOGGER_ENABLE_VESC" до #include "logger_codes.h".        */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_VESC
    { LOG_CODE_VESC_INIT_OK,       LOGGER_PRIORITY_LOW,    "Init - веска зарегистрирована" },
    { LOG_CODE_VESC_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "конфиг/пул/фильтр отклонён" },
    { LOG_CODE_VESC_REG_REJECTED,  LOGGER_PRIORITY_MEDIUM, "статус отклонён" },
    { LOG_CODE_VESC_EXIST_TIMEOUT, LOGGER_PRIORITY_MEDIUM, "RequestExists - таймаут ответа PONG" },
    { LOG_CODE_VESC_EXIST_OK, LOGGER_PRIORITY_LOW, "веска ответила PONG" },
    { LOG_CODE_VESC_CTRL_MODE, LOGGER_PRIORITY_LOW, "смена режима (value=1..10)" },
    { LOG_CODE_VESC_SEND_FAIL,     LOGGER_PRIORITY_MEDIUM, "отказ отправки CAN (value=cmd<<8|HAL)" },
    { LOG_CODE_VESC_RX_BAD_LEN,    LOGGER_PRIORITY_MEDIUM, "статус короче 8 байт" },
    { LOG_CODE_VESC_BAD_VALUE,     LOGGER_PRIORITY_LOW,    "NaN/вне диапазона в команде" },
    { LOG_CODE_VESC_MISUSE,        LOGGER_PRIORITY_LOW,    "неверное использование API (value=код)" },
    { LOG_CODE_VESC_POSMEM,        LOGGER_PRIORITY_LOW,    "нет валидной памяти положения" },
    { LOG_CODE_VESC_LOG_SUPPRESSED, LOGGER_PRIORITY_LOW,   "подавлено повторов записи (src=код)" },
    { LOG_CODE_VESC_BR_INIT_OK, LOGGER_PRIORITY_LOW, "bridge: Init - мост создан" },
    { LOG_CODE_VESC_BR_INIT_FAIL, LOGGER_PRIORITY_HIGH, "bridge: Init - отказ (пул/фильтр)" },
    { LOG_CODE_VESC_BR_MCCONF_READ, LOGGER_PRIORITY_LOW, "bridge: чтение MCCONF (value=размер)" },
    { LOG_CODE_VESC_BR_MCCONF_WRITE, LOGGER_PRIORITY_LOW, "bridge: запись MCCONF вески" },
    { LOG_CODE_VESC_BR_APPCONF_READ, LOGGER_PRIORITY_LOW, "bridge: чтение APPCONF" },
    { LOG_CODE_VESC_BR_APPCONF_WRITE, LOGGER_PRIORITY_LOW, "bridge: запись APPCONF" },
    { LOG_CODE_VESC_BR_FW_BOOTLOADER, LOGGER_PRIORITY_MEDIUM, "bridge: JUMP_TO_BOOTLOADER вески" },
    { LOG_CODE_VESC_BR_FW_ERASE, LOGGER_PRIORITY_MEDIUM, "bridge: ERASE_NEW_APP (старт обновления)" },
    { LOG_CODE_VESC_BR_FW_WRITE, LOGGER_PRIORITY_LOW, "bridge: запись прошивки (value=байты)" },
    { LOG_CODE_VESC_BR_FW_ERROR, LOGGER_PRIORITY_HIGH, "bridge: ошибка вески при обновлении" },
    { LOG_CODE_VESC_BR_SCAN_START, LOGGER_PRIORITY_LOW, "bridge: скан CAN начат" },
    { LOG_CODE_VESC_BR_SCAN_FOUND, LOGGER_PRIORITY_LOW, "bridge: скан CAN - найдена веска (src=id)" },
    { LOG_CODE_VESC_BR_SCAN_DONE, LOGGER_PRIORITY_LOW, "bridge: скан CAN готов (value=число)" },
    { LOG_CODE_VESC_BR_REPLY_TIMEOUT, LOGGER_PRIORITY_MEDIUM, "bridge: нет ответа вески (value=COMM-код)" },
    { LOG_CODE_VESC_BR_QUEUE_OVERFLOW, LOGGER_PRIORITY_HIGH, "bridge: очередь форвардинга полна" },
    { LOG_CODE_VESC_BR_RX_ERROR, LOGGER_PRIORITY_MEDIUM, "bridge: нарушена рамка пакета клиента" },
    { LOG_CODE_VESC_BR_RX_TIMEOUT, LOGGER_PRIORITY_MEDIUM, "bridge: оборван пакет клиента" },
    { LOG_CODE_VESC_BR_CAN_CRC_ERROR, LOGGER_PRIORITY_MEDIUM, "bridge: ошибка CRC ответа вески по CAN" },
    { LOG_CODE_VESC_BR_FAULT, LOGGER_PRIORITY_HIGH, "bridge: fault вески изменился (value=код)" },
    { LOG_CODE_VESC_BR_INIT_BAD_CONFIG,  LOGGER_PRIORITY_HIGH,   "bridge: Init - конфиг отклонён" },
    { LOG_CODE_VESC_BR_INIT_POOL_FULL,   LOGGER_PRIORITY_HIGH,   "bridge: Init - пул мостов исчерпан" },
    { LOG_CODE_VESC_BR_INIT_FILTER_FAIL, LOGGER_PRIORITY_HIGH,   "bridge: Init - фильтр CAN отклонён" },
    { LOG_CODE_VESC_BR_FWD_TOO_BIG,      LOGGER_PRIORITY_MEDIUM, "bridge: команда клиента велика" },
    { LOG_CODE_VESC_BR_FWD_SEND_BUSY,    LOGGER_PRIORITY_LOW,    "bridge: очередь CAN занята, отложено" },
    { LOG_CODE_VESC_BR_FWD_SEND_ERROR,   LOGGER_PRIORITY_HIGH,   "bridge: CANMGR_Send вернул ошибку" },
    { LOG_CODE_VESC_BR_RX_CRC_ERROR,     LOGGER_PRIORITY_MEDIUM, "bridge: неверный CRC пакета клиента" },
    { LOG_CODE_VESC_BR_RX_BAD_LEN,       LOGGER_PRIORITY_MEDIUM, "bridge: неверная длина пакета клиента" },
    { LOG_CODE_VESC_BR_CAN_BAD_FRAME,    LOGGER_PRIORITY_MEDIUM, "bridge: короткий служебный CAN-кадр" },
    { LOG_CODE_VESC_BR_CAN_FILL_OVERFLOW, LOGGER_PRIORITY_MEDIUM, "bridge: кусок ответа вески вне буфера" },
    { LOG_CODE_VESC_BR_CAN_LEN_ERROR,    LOGGER_PRIORITY_MEDIUM, "bridge: ответ вески длиннее буфера" },
    { LOG_CODE_VESC_BR_FWD_BAD_FRAME,    LOGGER_PRIORITY_LOW,    "bridge: неверный формат CAN_FWD_FRAME" },
    { LOG_CODE_VESC_BR_TX_TOO_BIG,       LOGGER_PRIORITY_MEDIUM, "bridge: ответ клиенту больше буфера" },
#endif /* LOGGER_ENABLE_VESC */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки stm32_biss_irs (энкодер LENZ IRS,        */
    /*  BiSS-C). Диапазон - см. LOG_ADDR_BISS_IRS выше. Включается через   */
    /*  "#define LOGGER_ENABLE_BISS_IRS" до #include "logger_codes.h".     */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_BISS_IRS
    { LOG_CODE_BISS_IRS_INIT_BAD_CONFIG,    LOGGER_PRIORITY_HIGH,   "Init - неверная конфигурация" },
    { LOG_CODE_BISS_IRS_INIT_POOL_FULL,     LOGGER_PRIORITY_HIGH,   "Init - пул энкодеров исчерпан" },
    { LOG_CODE_BISS_IRS_INIT_CLOCK_RANGE,   LOGGER_PRIORITY_HIGH,   "клок недостижим на этом ядре" },
    { LOG_CODE_BISS_IRS_INIT_OK,            LOGGER_PRIORITY_LOW,    "Init - энкодер зарегистрирован" },
    { LOG_CODE_BISS_IRS_POLL_ACK_TIMEOUT,   LOGGER_PRIORITY_MEDIUM, "Poll - тайм-аут ожидания ACK" },
    { LOG_CODE_BISS_IRS_POLL_FRAME_INVALID, LOGGER_PRIORITY_MEDIUM, "кадр невалиден (CRC/ERR/WARN)" },
    { LOG_CODE_BISS_IRS_ZERO_HERE_REJECTED, LOGGER_PRIORITY_LOW,    "нет данных для калибровки" },
    { LOG_CODE_BISS_IRS_POLL_BAD_HANDLE,    LOGGER_PRIORITY_MEDIUM, "Poll - NULL/неверный хэндл" },
    { LOG_CODE_BISS_IRS_POLL_ENC_ERROR,     LOGGER_PRIORITY_MEDIUM, "энкодер сообщает ошибку (бит ERR)" },
    { LOG_CODE_BISS_IRS_POLL_ENC_WARNING,   LOGGER_PRIORITY_LOW,    "энкодер: предупреждение (бит WARN)" },
    { LOG_CODE_BISS_IRS_POLL_CRC_ERROR,     LOGGER_PRIORITY_MEDIUM, "Poll - CRC6 кадра не сошлась" },
    { LOG_CODE_BISS_IRS_POLL_FRAMING_ERROR, LOGGER_PRIORITY_MEDIUM, "Poll - ошибка START/CDS кадра" },
    { LOG_CODE_BISS_IRS_LINK_RECOVERED,     LOGGER_PRIORITY_LOW,    "связь восстановлена, кадры валидны" },
    { LOG_CODE_BISS_IRS_SET_BAD_HANDLE,     LOGGER_PRIORITY_MEDIUM, "Set - NULL/неверный хэндл" },
#endif /* LOGGER_ENABLE_BISS_IRS */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки lsm6dsx (accel+gyro LSM6DSx/ISM330DHCX). */
    /*  Диапазон - см. LOG_ADDR_LSM6DSX выше. Включается через             */
    /*  "#define LOGGER_ENABLE_LSM6DSX" до #include "logger_codes.h".      */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_LSM6DSX
    { LOG_CODE_LSM6DSX_INIT_OK,          LOGGER_PRIORITY_LOW,  "инициализирован" },
    { LOG_CODE_LSM6DSX_INIT_FAIL_WHOAMI, LOGGER_PRIORITY_HIGH, "WHO_AM_I не совпал" },
    { LOG_CODE_LSM6DSX_BUS_ERROR,        LOGGER_PRIORITY_HIGH, "ошибка шины SPI/I2C" },
    { LOG_CODE_LSM6DSX_POOL_EXHAUSTED,   LOGGER_PRIORITY_HIGH, "пул экземпляров исчерпан" },
#endif /* LOGGER_ENABLE_LSM6DSX */
};

/** Количество записей в LOGGER_LogTable - используется LOGGER_Init(). */
#define LOGGER_LOG_TABLE_SIZE ((uint32_t)(sizeof(LOGGER_LogTable) / sizeof(LOGGER_LogTable[0])))

/* ------------------------------------------------------------------------ */
/*  Имена групп кодов (для текстового вывода) - таблица "адрес -> имя".     */
/*  Имя - РОВНО 8 символов ЗАГЛАВНЫМИ буквами (короткие добиты пробелами    */
/*  справа). В память/флеш не пишется: группа определяется старшим байтом   */
/*  кода. Группы, не входящие в библиотеку (загрузчик, проект), добавляются  */
/*  через LOGGER_EXTRA_GROUP_NAMES до #include "logger_codes.h", например:  */
/*      #define LOGGER_EXTRA_GROUP_NAMES { 0x02U, "BOOTLDR " },             */
/* ------------------------------------------------------------------------ */

typedef struct
{
    uint8_t addr;      /**< старший байт кода (LOG_ADDR_*) */
    char    name[9];   /**< ровно 8 символов + '\0' */
} LOGGER_GroupName_t;

static const LOGGER_GroupName_t LOGGER_GroupNames[] =
{
    { 0x00U, "LOGGER  " },
    { LOG_ADDR_SYSTEM, "SYSTEM  " },
#ifdef LOGGER_ENABLE_REG
    { LOG_ADDR_REG, "REGISTRY" },
#endif
#ifdef LOGGER_ENABLE_CANMGR
    { LOG_ADDR_CANMGR, "CAN_MGR " },
#endif
#ifdef LOGGER_ENABLE_USB_ETH
    { LOG_ADDR_USB_ETH, "USB_ETH " },
#endif
#ifdef LOGGER_ENABLE_USB_DEV
    { LOG_ADDR_USB_DEV, "USB_DEV " },
#endif
#ifdef LOGGER_ENABLE_LOAD_PWM
    { LOG_ADDR_LOAD_PWM, "LOAD_PWM" },
#endif
#ifdef LOGGER_ENABLE_RC_BUS
    { LOG_ADDR_RC_BUS, "RC_BUS  " },
#endif
#ifdef LOGGER_ENABLE_VESC_SERVO
    { LOG_ADDR_VESC_SERVO, "VESC_SRV" },
#endif
#ifdef LOGGER_ENABLE_VESC
    { LOG_ADDR_VESC, "VESC    " },
#endif
#ifdef LOGGER_ENABLE_BISS_IRS
    { LOG_ADDR_BISS_IRS, "BISS_IRS" },
#endif
#ifdef LOGGER_ENABLE_LSM6DSX
    { LOG_ADDR_LSM6DSX, "LSM6DSX " },
#endif
#ifdef LOGGER_EXTRA_GROUP_NAMES
    LOGGER_EXTRA_GROUP_NAMES
#endif
};

/** @brief Имя группы по коду лога (8 символов + '\0').
 * @param  code код лога (старший байт - адрес группы)
 * @return указатель на имя; "????????", если группа не в таблице */
static inline const char *LOGGER_GetGroupName(uint16_t code)
{
    uint8_t addr = (uint8_t)(code >> 8);

    for (uint32_t i = 0U; i < (uint32_t)(sizeof(LOGGER_GroupNames) / sizeof(LOGGER_GroupNames[0])); i++)
    {
        if (LOGGER_GroupNames[i].addr == addr)
        {
            return LOGGER_GroupNames[i].name;
        }
    }
    return "????????";
}
#endif /* LOGGER_CODES_H */
