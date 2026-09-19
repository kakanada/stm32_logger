/**
 ******************************************************************************
 * @file    logger_codes.h
 * @brief   Таблица кодов логов проекта - коды, приоритеты, текстовые описания.
 *          Этот файл заполняется индивидуально под каждый проект и является
 *          ЕДИНСТВЕННЫМ файлом, общим между встраиваемой (клиентской,
 *          logger.h/logger.c, пишет логи на STM32) и хостовой (серверной,
 *          log_decoder.h/log_decoder.c, читает логи на ПК/сервере) частями
 *          библиотеки - подключает только logger_types.h (портируемый, без
 *          HAL-зависимостей), поэтому собирается одинаково с обеих сторон.
 *          Сама библиотека не содержит ни одного пользовательского кода лога
 *          - все коды определяет пользователь библиотеки здесь.
 * @author  Mechanic
 * @date    19.09.2026
 * @version 1.6
 *
 * @copyright Copyright (c) 2026 Mechanic.
 *            Свободное некоммерческое использование и модификация. Условия
 *            распространения - см. LICENSE / README.md в составе проекта.
 ******************************************************************************
 */

/*
 * ============================================================================
 *  РЕКОМЕНДАЦИЯ ПО НУМЕРАЦИИ КОДОВ ЛОГОВ
 * ============================================================================
 *
 *  Код лога - число разрядностью 16 бит, указывается в HEX-формате
 *  (0xNNNN). Старший байт кода - адресное пространство логической группы
 *  источников логов, младший байт - номер конкретного события внутри
 *  группы. Старший байт разбит на 4 крупные КАТЕГОРИИ (в этом порядке по
 *  возрастанию адреса) - категория определяет, где искать/добавлять новую
 *  группу, и не даёт разным типам источников логов перемешиваться:
 *
 *      0x00xx-0x3Fxx  -  СИСТЕМА и внутренние алгоритмы проекта (общая
 *                        логика, память/EEPROM как таковая, любые чисто
 *                        программные подсистемы без привязки к конкретной
 *                        периферии или внешнему устройству)
 *      0x40xx-0x7Fxx  -  ПЕРИФЕРИЯ - интерфейсы МК как таковые (SPI, CAN,
 *                        I2C, UART, ...) и обёртки/менеджеры над ними
 *      0x80xx-0xDFxx  -  ВНЕШНИЕ УСТРОЙСТВА/ДАТЧИКИ, подключённые через
 *                        периферию (например VESC, W25Qxx, LOAD_PWM,
 *                        VESC_SERVO, энкодеры, IMU и т.п.)
 *      0xE0xx-0xFFxx  -  РЕЗЕРВ (пока не используется - на случай, если
 *                        три категории выше исчерпаются)
 *
 *  Внутри каждой категории группы нумеруются просто по порядку выделения
 *  (0x01, 0x02, ... от начала категории) - никакой дополнительной структуры
 *  внутри категории не требуется.
 *
 *  Для КАЖДОЙ группы (в том числе собственных групп проекта, не только
 *  зависимых библиотек - см. раздел про интеграцию ниже) адресное
 *  пространство и коды выражаются через триаду define:
 *  "LOG_ADDR_<ГРУППА>" (старший байт) -> "LOG_OFFSET_<ГРУППА>_<СОБЫТИЕ>"
 *  (младший байт) -> "LOG_CODE_<ГРУППА>_<СОБЫТИЕ>" (итоговый код,
 *  собранный из двух предыдущих). Именно "LOG_CODE_..." используется в
 *  LOGGER_LogTable ниже, а не число напрямую - перенос группы в другое
 *  адресное пространство тогда правится ОДНОЙ строкой ("LOG_ADDR_...") и
 *  НЕ требует уведомлять зависимые библиотеки (они ссылаются только на
 *  символьные LOG_CODE_..., не на числа).
 *
 *  Если один и тот же код может прийти от нескольких равноправных
 *  экземпляров одного модуля (например, несколько шин SPI или несколько
 *  однотипных датчиков) - НЕ заводите под каждый экземпляр отдельный код,
 *  используйте параметр source_id в LOGGER_Log() (см. logger.h) - код лога
 *  один на тип события, source_id отличает конкретный экземпляр.
 * ----------------------------------------------------------------------------
 *  ЗАРЕЗЕРВИРОВАННЫЙ ДИАПАЗОН - НЕ ИСПОЛЬЗОВАТЬ
 * ----------------------------------------------------------------------------
 *
 *  Диапазон 0x0000-0x00FF (старший байт кода 0x00) зарезервирован под
 *  служебные логи самой библиотеки (LOGGER_INTERNAL_CODE_INIT/FLUSH/MARK,
 *  см. logger.h) - LOGGER_Init() отклонит таблицу, если в ней есть код из
 *  этого диапазона. Начинайте нумерацию своих групп с 0x01xx.
 * ----------------------------------------------------------------------------
 *  ОБЯЗАТЕЛЬНОЕ ТРЕБОВАНИЕ К ТАБЛИЦЕ НИЖЕ
 * ----------------------------------------------------------------------------
 *
 *  Записи ДОЛЖНЫ идти строго по возрастанию поля code, без повторов -
 *  LOGGER_Init() проверяет это при старте (двоичный поиск по коду лога в
 *  LOGGER_Log() работает корректно и быстро только на отсортированной
 *  таблице). Нарушение порядка или повтор кода -> LOGGER_Init() вернёт
 *  HAL_ERROR.
 *
 *  Текстовое описание - только для человека (вывод в консоль/SWO), в
 *  энергонезависимую память не попадает. Рекомендуемая длина - 32-64
 *  символа, жёсткий предел - LOGGER_MAX_DESCRIPTION_LENGTH (см. logger.h).
 * ============================================================================
 *  ПРАВИЛА ИНТЕГРАЦИИ С БИБЛИОТЕКАМИ, ЗАВИСЯЩИМИ ОТ stm32_logger
 * ============================================================================
 *
 *  Если какая-то другая библиотека проекта хочет логировать через
 *  stm32_logger, её коды НЕ прописываются внутри самой этой зависимой
 *  библиотеки - они резервируются и живут ЗДЕСЬ, в этом файле, в едином
 *  для всего проекта месте. Это единственный способ не допустить, чтобы
 *  коды логов оказались раскиданы по репозиториям разных библиотек и
 *  случайно не пересеклись.
 *
 *  Порядок действий (со стороны автора stm32_logger / того, кто ведёт
 *  этот файл в конкретном проекте):
 *   1. Зависимая библиотека сообщает о своей потребности логировать через
 *      stm32_logger.
 *   2. Ей выделяется отдельный, ни с кем не пересекающийся диапазон
 *      старшего байта кода - внутри категории "ПЕРИФЕРИЯ" (0x40xx-0x7Fxx),
 *      если это сам интерфейс/шина, либо внутри категории "ВНЕШНИЕ
 *      УСТРОЙСТВА" (0x80xx-0xDFxx), если это конкретное устройство/датчик,
 *      подключённое через периферию. Диапазон она не выбирает сама.
 *   3. У зависимой библиотеки запрашивается ПОЛНЫЙ список того, что ей
 *      нужно логировать: код (в пределах выделенного диапазона),
 *      приоритет, короткое текстовое описание - для каждого события.
 *   4. Всё это заносится ниже отдельным блоком: сам блок записей таблицы
 *      И блок #define с символьными именами кодов - оба блока под ОДНИМ
 *      и тем же условием "#ifdef LOGGER_ENABLE_<ИМЯ>".
 *
 *  Правила оформления каждого такого блока (см. пример ниже, LOAD_PWM):
 *   - Блок записей таблицы оборачивается в "#ifdef LOGGER_ENABLE_<ИМЯ>" /
 *     "#endif" ПРЯМО ВНУТРИ инициализатора LOGGER_LogTable - это обычный
 *     приём условной компиляции внутри списка инициализации в C, ничего
 *     специфичного для этого проекта.
 *   - Перед таблицей (выше LOGGER_LogTable) - три уровня define этой
 *     библиотеки, тоже под тем же "#ifdef LOGGER_ENABLE_<ИМЯ>":
 *       1) "#define LOG_ADDR_<ИМЯ> 0xNNU" - старший байт (адресное
 *          пространство), выделенный этой библиотеке. Меняется ОДНОЙ
 *          строкой при необходимости перевыделить диапазон.
 *       2) "#define LOG_OFFSET_<ИМЯ>_<СОБЫТИЕ> 0x00U" (0x01U, 0x02U, ...) -
 *          младший байт, номер события внутри диапазона библиотеки.
 *       3) "#define LOG_CODE_<ИМЯ>_<СОБЫТИЕ> ((uint16_t)((LOG_ADDR_<ИМЯ> << 8) | LOG_OFFSET_<ИМЯ>_<СОБЫТИЕ>))" -
 *          итоговый код, собранный из двух define выше. Именно
 *          LOG_CODE_<ИМЯ>_<СОБЫТИЕ> используется и в записи таблицы
 *          LOGGER_LogTable, и передаётся зависимой библиотеке.
 *   - Зависимая библиотека в своём коде НИКОГДА не пишет код лога как
 *     число (0xNNxx) напрямую - только через выданный ей
 *     LOG_CODE_<ИМЯ>_<СОБЫТИЕ>. Так код зависимой библиотеки не хранит
 *     у себя то, что по правилам должно жить в этом файле, а смена
 *     адресного пространства библиотеки - это правка одного define
 *     LOG_ADDR_<ИМЯ> здесь, без единой правки в самой зависимой
 *     библиотеке и без уведомления её автора.
 *   - Если "LOGGER_ENABLE_<ИМЯ>" не определён - ни записи таблицы, ни
 *     define'ы этой библиотеки не попадают в сборку вовсе. Пользователь
 *     проекта включает логи конкретной зависимой библиотеки одним
 *     "#define LOGGER_ENABLE_<ИМЯ>" (до "#include \"logger_codes.h\"")
 *     и выключает точно так же - удалением/закомментированием этого
 *     define. Это и защита от бесконечного роста общей таблицы (в
 *     сборке остаётся только то, что действительно используется), и
 *     единая точка, где выключить логи ненужной сейчас зависимости.
 * ============================================================================
 */

#ifndef LOGGER_CODES_H
#define LOGGER_CODES_H

#include "logger_types.h"

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

/* ------------------------------------------------------------------ */
/*  ПРИМЕР - собственная группа проекта (замените на реальную).       */
/* ------------------------------------------------------------------ */
/* Адресное пространство группы "SPI". */
#define LOG_ADDR_SPI 0x40U
#define LOG_OFFSET_SPI_RETRY 0x01U
#define LOG_CODE_SPI_RETRY ((uint16_t)((LOG_ADDR_SPI << 8) | LOG_OFFSET_SPI_RETRY))

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

/* ------------------------------------------------------------------ */
/*  ПРИМЕР - собственная группа проекта (замените на реальную).       */
/* ------------------------------------------------------------------ */
/* Адресное пространство группы "внешняя память" (например, W25Qxx). */
#define LOG_ADDR_MEMORY 0x80U
#define LOG_OFFSET_MEMORY_WRITE_TIMEOUT 0x01U
#define LOG_OFFSET_MEMORY_CRC_ERROR     0x02U
#define LOG_CODE_MEMORY_WRITE_TIMEOUT ((uint16_t)((LOG_ADDR_MEMORY << 8) | LOG_OFFSET_MEMORY_WRITE_TIMEOUT))
#define LOG_CODE_MEMORY_CRC_ERROR     ((uint16_t)((LOG_ADDR_MEMORY << 8) | LOG_OFFSET_MEMORY_CRC_ERROR))

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
    /* ------------------------------------------------------------------ */
    /*  ПРИМЕР - замените на реальные коды логов своего проекта.          */
    { LOG_CODE_SPI_RETRY, LOGGER_PRIORITY_LOW, "SPI: повторная попытка передачи" },

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки can_manager (CANMGR). Диапазон -         */
    /*  см. LOG_ADDR_CANMGR выше. Включается через                        */
    /*  "#define LOGGER_ENABLE_CANMGR" до #include "logger_codes.h".       */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_CANMGR
    { LOG_CODE_CANMGR_INIT_OK,       LOGGER_PRIORITY_LOW,    "can_manager: Init - шина инициализирована" },
    { LOG_CODE_CANMGR_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "can_manager: Init - ошибка конфигурации/пула/HAL" },
    { LOG_CODE_CANMGR_REG_REJECTED,  LOGGER_PRIORITY_MEDIUM, "can_manager: RegisterFilter - регистрация фильтра отклонена" },
    { LOG_CODE_CANMGR_TX_QUEUE_FULL, LOGGER_PRIORITY_HIGH,   "can_manager: Send/SendLatest - очередь отправки переполнена" },
    { LOG_CODE_CANMGR_RX_OVERFLOW,   LOGGER_PRIORITY_MEDIUM, "can_manager: переполнение Rx FIFO0" },
    { LOG_CODE_CANMGR_BUS_OFF,       LOGGER_PRIORITY_HIGH,   "can_manager: Bus-Off обнаружен и автовосстановлен" },
#endif /* LOGGER_ENABLE_CANMGR */

    /* ------------------------------------------------------------------ */
    /*  ВНЕШНИЕ УСТРОЙСТВА/ДАТЧИКИ на периферии (0x80xx-0xDFxx).           */
    /* ------------------------------------------------------------------ */
    /*  ПРИМЕР - замените на реальные коды логов своего проекта.          */
    { LOG_CODE_MEMORY_WRITE_TIMEOUT, LOGGER_PRIORITY_MEDIUM, "Внешняя память: тайм-аут записи" },
    { LOG_CODE_MEMORY_CRC_ERROR,     LOGGER_PRIORITY_HIGH,   "Внешняя память: сбой CRC при чтении" },

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки LOAD_PWM (stm32_load_pwm). Диапазон -    */
    /*  см. LOG_ADDR_LOAD_PWM выше. Включается через                      */
    /*  "#define LOGGER_ENABLE_LOAD_PWM" до #include "logger_codes.h".     */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_LOAD_PWM
    { LOG_CODE_LOAD_PWM_INIT_BAD_CONFIG,    LOGGER_PRIORITY_HIGH,   "LOAD_PWM: Init - неверная конфигурация" },
    { LOG_CODE_LOAD_PWM_INIT_POOL_FULL,     LOGGER_PRIORITY_HIGH,   "LOAD_PWM: Init - пул нагрузок исчерпан" },
    { LOG_CODE_LOAD_PWM_INIT_LED_POOL_FULL, LOGGER_PRIORITY_HIGH,   "LOAD_PWM: Init - пул LED-таблиц гаммы исчерпан" },
    { LOG_CODE_LOAD_PWM_INIT_HAL_START_FAIL, LOGGER_PRIORITY_HIGH,  "LOAD_PWM: Init - HAL_TIM_PWM_Start() вернул ошибку" },
    { LOG_CODE_LOAD_PWM_INIT_OK,             LOGGER_PRIORITY_LOW,   "LOAD_PWM: Init - успешная регистрация нагрузки" },
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
    { LOG_CODE_RC_BUS_FRAME_ERROR,            LOGGER_PRIORITY_LOW,    "rc_bus: битый/нераспознанный кадр каналов" },
    { LOG_CODE_RC_BUS_TELEMETRY_INIT_FAIL,    LOGGER_PRIORITY_HIGH,   "rc_bus: TelemetryInit - конфигурация отклонена" },
    { LOG_CODE_RC_BUS_TELEMETRY_UART_ERROR,   LOGGER_PRIORITY_MEDIUM, "rc_bus: ошибка приёма на шине датчиков" },
    { LOG_CODE_RC_BUS_TELEMETRY_FRAME_ERROR,  LOGGER_PRIORITY_LOW,    "rc_bus: битый кадр опроса на шине датчиков" },
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
    { LOG_CODE_VESC_SERVO_INIT_DUPLICATE,  LOGGER_PRIORITY_HIGH, "vesc_servo: Init - веска уже обёрнута другой сервой" },
    { LOG_CODE_VESC_SERVO_INIT_OK,         LOGGER_PRIORITY_LOW,  "vesc_servo: Init - серва успешно зарегистрирована" },
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
    { LOG_CODE_VESC_INIT_FAIL,     LOGGER_PRIORITY_HIGH,   "motor_vesc: Init - неверная конфигурация/пул исчерпан/фильтр отклонён" },
    { LOG_CODE_VESC_REG_REJECTED,  LOGGER_PRIORITY_MEDIUM, "motor_vesc: RegisterCustomStatus - регистрация отклонена" },
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
    { LOG_CODE_BISS_IRS_INIT_CLOCK_RANGE,   LOGGER_PRIORITY_HIGH,   "BISS_IRS: Init - частота клока недостижима на этом ядре" },
    { LOG_CODE_BISS_IRS_INIT_OK,            LOGGER_PRIORITY_LOW,    "BISS_IRS: Init - энкодер зарегистрирован" },
    { LOG_CODE_BISS_IRS_POLL_ACK_TIMEOUT,   LOGGER_PRIORITY_MEDIUM, "BISS_IRS: Poll - тайм-аут ожидания ACK" },
    { LOG_CODE_BISS_IRS_POLL_FRAME_INVALID, LOGGER_PRIORITY_MEDIUM, "BISS_IRS: Poll - кадр невалиден (CRC/ERR/WARN/framing)" },
    { LOG_CODE_BISS_IRS_ZERO_HERE_REJECTED, LOGGER_PRIORITY_LOW,    "BISS_IRS: ZeroHere - нет валидных данных для калибровки" },
#endif /* LOGGER_ENABLE_BISS_IRS */

    /* ------------------------------------------------------------------ */
    /*  Блок зависимой библиотеки lsm6dsx (accel+gyro LSM6DSx/ISM330DHCX). */
    /*  Диапазон - см. LOG_ADDR_LSM6DSX выше. Включается через             */
    /*  "#define LOGGER_ENABLE_LSM6DSX" до #include "logger_codes.h".      */
    /* ------------------------------------------------------------------ */
#ifdef LOGGER_ENABLE_LSM6DSX
    { LOG_CODE_LSM6DSX_INIT_OK,          LOGGER_PRIORITY_LOW,  "Датчик LSM6DSx успешно инициализирован" },
    { LOG_CODE_LSM6DSX_INIT_FAIL_WHOAMI, LOGGER_PRIORITY_HIGH, "Ошибка инициализации LSM6DSx: WHO_AM_I не совпал с ожидаемым" },
    { LOG_CODE_LSM6DSX_BUS_ERROR,        LOGGER_PRIORITY_HIGH, "Ошибка шины SPI/I2C при обращении к LSM6DSx" },
    { LOG_CODE_LSM6DSX_POOL_EXHAUSTED,   LOGGER_PRIORITY_HIGH, "Пул экземпляров LSM6DSx (LSM6DSX_MAX_DEVICES) исчерпан" },
#endif /* LOGGER_ENABLE_LSM6DSX */
};

/** Количество записей в LOGGER_LogTable - используется LOGGER_Init(). */
#define LOGGER_LOG_TABLE_SIZE ((uint32_t)(sizeof(LOGGER_LogTable) / sizeof(LOGGER_LogTable[0])))

#endif /* LOGGER_CODES_H */
