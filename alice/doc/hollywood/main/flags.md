# Флаги экспериментов

Сценарии Голливуда получают флаги экспериментов через методы класса `TRequest::TFlags()`. Флаги экспериментов могут иметь следующие форматы записи:
* `"key":nothing`
* `"key":"value"`
* `"key=subvalue":nothing`
* `"key=subvalue":"value"` (в этом случае значение `value` равно `0` или `1`)

Сценарий может выполнять следующие действия:
* проверять, установлен ли эксперимент;
* проверять, встречается ли ключ вместе с дополнительным значением;
* получать доступы к значениям `value`/`subvalue`;
* перебирать ключи в списке эксперимента.

## Установлен ли эксперимент

```cpp
bool TRequest::Flags::IsExperimentEnabled(const TStringBuf key) const;
```
Эксперимент считается включенным, если значение `value` определено и не равно `0`. Значение `key` должно передаваться в полном формате. 

| Значение эксперимента | Результат вызова функции                  |
| --------------------- | ----------------------------------        |
| ключи отсутствуют     | `IsExperimentEnabled("key")` — `false`        |
| `"key"`                 | `IsExperimentEnabled("key")` — `false`        |
| `"key":"0"`             | `IsExperimentEnabled("key")` — `false`        |
| `"key":"1"`             | `IsExperimentEnabled("key")` — `true`         |
| `"key":"xxx"`           | `IsExperimentEnabled("key")` — `true`         |
| `"key=subval"`          | `IsExperimentEnabled("key")` — `false`        |
| `"key=subval"`          | `IsExperimentEnabled("key=subval")` — `false` |
| `"key=subval":"0"`      | `IsExperimentEnabled("key")` — `false`        |
| `"key=subval":"0"`      | `IsExperimentEnabled("key=subval")` — `false` |
| `"key=subval":"1"`      | `IsExperimentEnabled("key")` — `false`        |
| `"key=subval":"1"`      | `IsExperimentEnabled("key=subval")` — `true`  |
| `"key=subval":"xxx"`    | `IsExperimentEnabled("key")` — `false`, при парсинге ключей будет выдано предупреждение |
| `"key=subval":"xxx"`    | `IsExperimentEnabled("key=subval")` — `true`  |

## Встречается ли ключ вместе с дополнительным значением

```cpp
bool TRequest::Flags::IsKeyComplex(const TStringBuf key) const;
```

| Значение эксперимента | Результат вызова функции                  |
| --------------------- | ----------------------------------        |
| отсутствие ключей     | `IsKeyComplex("key")` — `false`        |
| `"key"`                 | `IsKeyComplex("key")` — `false`        |
| `"key":"0"`             | `IsKeyComplex("key")` — `false`        |
| `"key=subval"`          | `IsKeyComplex("key")` — `true`         |
| `"key=subval"`          | `IsKeyComplex("key=subval")` — `false` |

```cpp
template <typename T>
TMaybe<T> GetValue(TStringBuf key) const;

template <typename T>
T GetValue(TStringBuf key, T defaultValue) const;
```

Обе функции имеют одинаковое поведение и возвращают либо `TMaybe` для значения, либо само значение или величину по умолчанию.

| Значение эксперимента | Результат вызова функции           |
| --------------------- | ---------------------------------- |
| ключи отсутствуют     | `GetValue("key")` — `Nothing()`        |
| `"key"`                 | `GetValue("key")` — `Nothing()`        |
| `"key":"0"`             | `GetValue("key")` — `TMaybe("0")`      |
| `"key":"1"`             | `GetValue("key")` — `TMaybe("1")`      |
| `"key":"xxx"`           | `GetValue("key")` — `TMaybe("xxx")`    |
| `"key=subval"`          | `GetValue("key")` — `Nothing()`        |
| `"key=subval"`          | `GetValue("key=subval")` — `Nothing()` |
| `"key=subval":"0"`      | `GetValue("key")` — `TMaybe("0")`      |

## Получить доступ к значению

Пользуйтесь этой функцией только если вы уверены, что ваш эксперимент задан на клиенте одним единственным способом.

```cpp
template <typename T>
TMaybe<T> GetSubValue(TStringBuf key) const;

template <typename T>
T GetSubValue(TStringBuf key, T defaultValue) const;
```

Обе функции возвращают один из вариантов:
* `TMaybe` для `subvalue`;
* значение или величину по умолчанию.

Если значение основного поля `value` — `Nothing` или `"0"`, функция вернет `Nothing()` вместо ключа `subvalue`.

Если у вас задано несколько экспериментов вида `"key1=subval1"`, `"key1=subval2"`, то запрос `GetSubValue("key1")` вернет какое-то из двух значений.

## Перебрать ключи в списке эксперимента

```cpp
template <typename T>
template <typename T>
bool ForEach(T obj, std::function<bool(T obj, const TString& key, const TMaybe<TString>& value)> fn) const;

template <typename T>
bool ForEach(T obj, std::function<bool(T obj, const TString& key, const TString& subval, const TMaybe<TString>& value)> fn) const;                                                                                                                                 
```

Поле `T` позволяет передать в лямбду любое дополнительное значение для внутренних операций.

Первая функция используется для варианта `"key":"value"` или `"key=subvalue":"value"`. Вторая функция используется для варианта `"key=subval":"value"`. Она возвращает все эксперименты, в которых имеется знак присваивания в `key`. Обе функции возвращают все эксперименты, независимо от типа переменной `value`.

Если лямбда вернула `false`, перебор ключей прекращается, метод `ForEach()` возвращает `false`. Если перебор ключей закончился, метод `ForEach()` возвращает `true`.


