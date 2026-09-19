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
 * @version 1.5
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
 *  (0xNNNN). Рекомендуется резервировать под каждую логическую группу
 *  источников логов старший байт кода, а младший байт использовать для
 *  нумерации конкретных событий внутри группы, например:
 *
 *      0x01xx  -  ошибки/события общей логики системы
 *      0x02xx  -  ошибки/события микросхемы внешней памяти
 *      0x03xx  -  ошибки/события работы с интерфейсом SPI
 *      0x04xx  -  ошибки/события работы с интерфейсом CAN
 *      0x05xx  -  ошибки/события работы с интерфейсом UART
 *      ...        и так далее - по одной группе на каждый логический
 *                 источник логов конкретного проекта
 *
 *  Такое разделение уже по самому коду лога позволяет определить источник
 *  события без обращения к этой таблице (например, при просмотре дампа
 *  энергонезависимой памяти) и снижает риск случайного совпадения кодов
 *  между разными модулями прошивки.
 *
 *  Для КАЖДОЙ группы (в том числе собственных групп проекта, не только
 *  зависимых библиотек - см. раздел про интеграцию ниже) адресное
 *  пространство и коды выражаются через триаду define:
 *  "LOG_ADDR_<ГРУППА>" (старший байт) -> "LOG_OFFSET_<ГРУППА>_<СОБЫТИЕ>"
 *  (младший байт) -> "LOG_CODE_<ГРУППА>_<СОБЫТИЕ>" (итоговый код,
 *  собранный из двух предыдущих). Именно "LOG_CODE_..." используется в
 *  LOGGER_LogTable ниже, а не число напрямую - перенос группы в другое
 *  адресное пространство тогда правится ОДНОЙ строкой ("LOG_ADDR_...").
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
 *      старшего байта кода (0x0Nxx), который она не выбирает сама.
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
 *       1) "#define LOG_ADDR_<ИМЯ> 0x0NU" - старший байт (адресное
 *          пространство), выделенный этой библиотеке. Меняется ОДНОЙ
 *          строкой при необходимости перевыделить диапазон.
 *       2) "#define LOG_OFFSET_<ИМЯ>_<СОБЫТИЕ> 0x00U" (0x01U, 0x02U, ...) -
 *          младший байт, номер события внутри диапазона библиотеки.
 *       3) "#define LOG_CODE_<ИМЯ>_<СОБЫТИЕ> ((uint16_t)((LOG_ADDR_<ИМЯ> << 8) | LOG_OFFSET_<ИМЯ>_<СОБЫТИЕ>))" -
 *          итоговый код, собранный из двух define выше. Именно
 *          LOG_CODE_<ИМЯ>_<СОБЫТИЕ> используется и в записи таблицы
 *          LOGGER_LogTable, и передаётся зависимой библиотеке.
 *   - Зависимая библиотека в своём коде НИКОГДА не пишет код лога как
 *     число (0x0Dxx) напрямую - только через выданный ей
 *     LOG_CODE_<ИМЯ>_<СОБЫТИЕ>. Так код зависимой библиотеки не хранит
 *     у себя то, что по правилам должно жить в этом файле, а смена
 *     адресного пространства библиотеки - это правка одного define
 *     LOG_ADDR_<ИМЯ> здесь, без единой правки в самой зависимой
 *     библиотеке.
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
/* ---------------------------------------------------------------------- */

/* ------------------------------------------------------------------ */
/*  ПРИМЕР - собственные группы проекта (замените на реальные).       */
/* ------------------------------------------------------------------ */
/* Адресное пространство группы "система". */
#define LOG_ADDR_SYSTEM 0x01U
#define LOG_OFFSET_SYSTEM_START     0x01U
#define LOG_OFFSET_SYSTEM_HAL_ERROR 0x02U
#define LOG_CODE_SYSTEM_START     ((uint16_t)((LOG_ADDR_SYSTEM << 8) | LOG_OFFSET_SYSTEM_START))
#define LOG_CODE_SYSTEM_HAL_ERROR ((uint16_t)((LOG_ADDR_SYSTEM << 8) | LOG_OFFSET_SYSTEM_HAL_ERROR))

/* Адресное пространство группы "память". */
#define LOG_ADDR_MEMORY 0x02U
#define LOG_OFFSET_MEMORY_WRITE_TIMEOUT 0x01U
#define LOG_OFFSET_MEMORY_CRC_ERROR     0x02U
#define LOG_CODE_MEMORY_WRITE_TIMEOUT ((uint16_t)((LOG_ADDR_MEMORY << 8) | LOG_OFFSET_MEMORY_WRITE_TIMEOUT))
#define LOG_CODE_MEMORY_CRC_ERROR     ((uint16_t)((LOG_ADDR_MEMORY << 8) | LOG_OFFSET_MEMORY_CRC_ERROR))

/* Адресное пространство группы "SPI". */
#define LOG_ADDR_SPI 0x03U
#define LOG_OFFSET_SPI_RETRY 0x01U
#define LOG_CODE_SPI_RETRY ((uint16_t)((LOG_ADDR_SPI << 8) | LOG_OFFSET_SPI_RETRY))

#ifdef LOGGER_ENABLE_LOAD_PWM
/* Адресное пространство (старший байт кода), выделенное LOAD_PWM. */
#define LOG_ADDR_LOAD_PWM 0x0DU

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

static const LOGGER_LogEntry_t LOGGER_LogTable[] =
{
    /* ------------------------------------------------------------------ */
    /*  ПРИМЕР - замените на реальные коды логов своего проекта.          */
    /*    code                        priority                   description */
    /* ------------------------------------------------------------------ */
    { LOG_CODE_SYSTEM_START,        LOGGER_PRIORITY_LOW,    "Система: штатный старт" },
    { LOG_CODE_SYSTEM_HAL_ERROR,    LOGGER_PRIORITY_HIGH,   "Система: сбой инициализации HAL" },
    { LOG_CODE_MEMORY_WRITE_TIMEOUT, LOGGER_PRIORITY_MEDIUM, "Память: тайм-аут записи" },
    { LOG_CODE_MEMORY_CRC_ERROR,     LOGGER_PRIORITY_HIGH,   "Память: сбой CRC при чтении" },
    { LOG_CODE_SPI_RETRY,            LOGGER_PRIORITY_LOW,    "SPI: повторная попытка передачи" },

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
};

/** Количество записей в LOGGER_LogTable - используется LOGGER_Init(). */
#define LOGGER_LOG_TABLE_SIZE ((uint32_t)(sizeof(LOGGER_LogTable) / sizeof(LOGGER_LogTable[0])))

#endif /* LOGGER_CODES_H */
