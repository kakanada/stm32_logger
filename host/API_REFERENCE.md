# logger (host) - справочник по API

Дисклеймер: при расхождениях с `log_decoder.h` ориентируйтесь на него, он первичен, этот справочник - его сжатый пересказ.

## Оглавление

- [Разбор дампа памяти](#разбор-дампа-памяти)
- [Постфактум-статистика частоты](#постфактум-статистика-частоты)

---

## Разбор дампа памяти

Функции и тип ниже - из `log_decoder.h`, хостовая часть (ПК/сервер). Не зависят от встраиваемой части и не собираются вместе с ней - общий только `logger_codes.h`.

### `LOGDEC_DecodedRecord_t`

Результат разбора одной `LOGGER_Record_t` - сырые поля записи плюс уже подставленные приоритет и описание из таблицы кодов.

| Поле | Тип | Описание |
|---|---|---|
| `code` | `uint16_t` | код лога, как в дампе памяти |
| `source_id` | `uint16_t` | идентификатор источника события |
| `value` | `int32_t` | значение переменной |
| `systick` | `uint32_t` | SysTick на момент события |
| `rtc_time` | `uint32_t` | показание RTC на момент события, либо 0 |
| `priority` | `LOGGER_Priority_t` | приоритет из таблицы кодов; `LOGGER_PRIORITY_HIGH`, если код не найден |
| `description` | `const char*` | описание из таблицы кодов; `NULL`, если код не найден ни в одной таблице |

### `bool LOGDEC_Decode(const uint8_t *data, size_t data_length, LOGDEC_DecodedRecord_t *out_records, size_t out_capacity, size_t *out_count)`

Разбирает сырой дамп памяти (массив `LOGGER_Record_t` подряд) целиком в предоставленный буфер `out_records`. "Всё или ничего": если `out_capacity` не хватает на все записи дампа - функция возвращает `false` и не пишет в `out_records` частично.

Возвращает `true` - дамп полностью разобран, `out_records`/`*out_count` (если не `NULL`) заполнены; `false` - `data`/`out_records == NULL`, `data_length` не кратна `sizeof(LOGGER_Record_t)` (повреждённый/обрезанный дамп), либо `out_capacity` не хватает.

### `const char *LOGDEC_GetDescription(uint16_t code)`

Точечный поиск текстового описания по коду - из пользовательской таблицы (`logger_codes.h`) либо, для диапазона `0x0000-0x00FF`, из служебной таблицы библиотеки. Возвращает `NULL`, если код не найден ни в одной из таблиц.

### `bool LOGDEC_GetPriority(uint16_t code, LOGGER_Priority_t *out_priority)`

Точечный поиск приоритета по коду, из тех же таблиц, что и `LOGDEC_GetDescription()`. Возвращает `true` и заполняет `*out_priority`, если код найден; `false` - если `out_priority == NULL` либо код не найден ни в одной таблице.

---

## Постфактум-статистика частоты

Хостовый аналог `LOGGER_GetCodeFrequency()`/`LOGGER_GetMarkFrequency()` - считается сразу по всему массиву записей, разобранному `LOGDEC_Decode()`, а не в реальном времени. Обе функции, как и `LOGDEC_Decode()`, "всё или ничего": если `out_capacity` не хватает на все различные коды/метки - возвращают `false` и не пишут в `out_stats`.

### `bool LOGDEC_ComputeCodeStats(const LOGDEC_DecodedRecord_t *records, size_t record_count, LOGDEC_CodeStats_t *out_stats, size_t out_capacity, size_t *out_count)`

Статистика по каждому отдельному коду в `records` (кроме `LOGGER_INTERNAL_CODE_MARK` - см. `LOGDEC_ComputeMarkStats()`). `LOGDEC_CodeStats_t`: `code`, `call_count`, `first_systick`, `last_systick`, `frequency_hz` (0.0f, если данных недостаточно).

### `bool LOGDEC_ComputeMarkStats(const LOGDEC_DecodedRecord_t *records, size_t record_count, LOGDEC_MarkStats_t *out_stats, size_t out_capacity, size_t *out_count)`

Статистика по каждой отдельной метке (`LOGGER_Mark(mark_id)`) - находит записи с кодом `LOGGER_INTERNAL_CODE_MARK` и агрегирует их по `source_id` (= `mark_id`). `LOGDEC_MarkStats_t`: `mark_id`, `call_count`, `first_systick`, `last_systick`, `frequency_hz`.

