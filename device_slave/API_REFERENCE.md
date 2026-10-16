# logger (device_slave) - справочник по API

Дисклеймер: при расхождениях с `logger.h` ориентируйтесь на него, он первичен, этот справочник - его сжатый пересказ.

## Оглавление

- [Конфигурация при компиляции](#конфигурация-при-компиляции)
- [Типы](#типы)
- [Функции](#функции)
- [Формат кадра на линии](#формат-кадра-на-линии)

---

## Конфигурация при компиляции

Переопределяется через `#define` ПЕРЕД `#include "logger.h"`.

| Define | По умолчанию | Описание |
|---|---|---|
| `LOGGER_BUFFER_CAPACITY` | `32` | ёмкость очереди кадров (12 байт ОЗУ каждый), 2..255 |
| `LOGGER_MAX_FRAMES_PER_PROCESS` | `4` | сколько кадров максимум отправляется за один `LOGGER_Process()` |
| `LOGGER_NO_ITM` | не определён | ядра без ITM (Cortex-M0/M0+): убирает вывод в SWO, `console_fn` обязательна |

---

## Типы

### `LOGGER_SendFn_t`

```c
typedef HAL_StatusTypeDef (*LOGGER_SendFn_t)(void *context, const LOGGER_Frame_t *frame);
```

Отправка одного кадра master по физическому интерфейсу, вызывается из `LOGGER_Process()`. `HAL_OK` - кадр принят интерфейсом и удалён из очереди; иное значение - кадр останется в очереди до следующего `LOGGER_Process()`.

### `LOGGER_ConsoleWriteFn_t`

```c
typedef void (*LOGGER_ConsoleWriteFn_t)(void *context, uint8_t node_id, uint16_t code, uint16_t source_id,
                                         LOGGER_Priority_t priority, int32_t value,
                                         uint32_t systick, uint32_t rtc_time,
                                         const char *description);
```

Вывод одной записи в консоль, заменяет SWO. `rtc_time` всегда 0, `node_id` - id этого slave.

### `LOGGER_Config_t`

| Поле | Тип | Описание |
|---|---|---|
| `node_id` | `uint8_t` | id этого slave, 1..255 |
| `send_fn` | `LOGGER_SendFn_t` | отправка master. `NULL` - только локальный вывод |
| `send_context` | `void*` | первый аргумент `send_fn` |
| `send_min_priority` | `LOGGER_Priority_t` | LOW (0) - отправлять всё; MEDIUM; HIGH - только ошибки |
| `console_fn` | `LOGGER_ConsoleWriteFn_t` | вывод в консоль, `NULL` - SWO |
| `console_context` | `void*` | первый аргумент `console_fn` |

### `LOGGER_Frame_t`

Определён в `logger_types.h`: `node_id`, `seq`, `code`, `source_id`, `value` - см. [Формат кадра на линии](#формат-кадра-на-линии).

---

## Функции

### `HAL_StatusTypeDef LOGGER_Init(const LOGGER_Config_t *config)`

Проверяет конфигурацию, обнуляет очередь и нумерацию кадров, выводит служебную запись об инициализации (только локально). `HAL_ERROR` - `config == NULL`, `node_id == 0`, неверный `send_min_priority`, либо (при `LOGGER_NO_ITM`) нет `console_fn`. Необязательна: без неё `LOGGER_Log()` только выводит в SWO.

### `void LOGGER_Log(uint16_t code, uint16_t source_id, int32_t value)`

Выводит запись в консоль/SWO и ставит кадр в очередь, если `send_fn` задана и приоритет кода не ниже `send_min_priority`. Неизвестный код выводится с описанием `???` и приоритетом HIGH и отправляется. Быстрая, неблокирующая, допустима из ISR. При полной очереди кадр теряется (номер `seq` расходуется, потеря считается).

### `uint8_t LOGGER_Process(void)`

Отправляет накопленные кадры через `send_fn`, не более `LOGGER_MAX_FRAMES_PER_PROCESS` за вызов; останавливается на первой неудаче. Возвращает число отправленных кадров. Вызывать из главного цикла.

### `uint32_t LOGGER_GetDroppedCount(void)`

Сколько кадров потеряно локально (очередь была полна) с момента `LOGGER_Init()`.

### `uint16_t LOGGER_GetPending(void)`

Сколько кадров ждёт отправки.

### `size_t LOGGER_FormatConsoleLine(char *buf, size_t size, uint8_t node_id, uint16_t code, uint16_t source_id, LOGGER_Priority_t priority, int32_t value, uint32_t systick, uint32_t rtc_time, const char *description)`

Формирует строку в формате `<RTC,19> <мс,8> <узел,3> <ГРУППА,8> <L|M|H> 0x<код> <src,4> <val,11> <описание>\r\n` (без добивки), как у master. Возврат - длина полной строки без `'\0'` (как у `snprintf`).

---

## Формат кадра на линии

Логический кадр - `LOGGER_Frame_t`:

| Поле | Тип | Описание |
|---|---|---|
| `node_id` | `uint8_t` | id slave |
| `seq` | `uint8_t` | номер кадра по модулю 256; расходуется и потерянными кадрами |
| `code` | `uint16_t` | код лога |
| `source_id` | `uint16_t` | идентификатор источника |
| `value` | `int32_t` | значение |

Упаковка (в `logger_types.h`, little-endian):

- `LOGGER_FramePackRaw()` - `LOGGER_WIRE_RAW_SIZE` = 10 байт: `node_id`, `seq`, `code`(2), `source_id`(2), `value`(4).
- `LOGGER_FramePackStream()` - `LOGGER_WIRE_STREAM_SIZE` = 13 байт: `0xA5`, 10 байт, CRC16-CCITT (поли 0x1021, начальное 0xFFFF) по 10 байтам, little-endian. Для UART/SPI; на приёмной стороне разбирается `LOGGER_StreamFeed()` у master.
- Classic CAN: расширенный идентификатор `(node_id << 8) | seq`, данные 8 байт = `code`(2), `source_id`(2), `value`(4).
