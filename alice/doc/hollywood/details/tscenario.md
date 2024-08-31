# Класс TScenario

Основной класс для реализации сценариев в Hollywood Framework. Основная часть методов используется только во время регистрации сценария.

## Конструктор

### Базовый конструктор сценария

```c++
explicit TScenario(const TString& name, bool enableDebugGraph = false);
```

Класс сценария при создании должен передать в него имя сценария. Имя записывается в `lower_case`-формате и должно соответствовать именованию в конфигурации Megaming и Apphost, например:

```c++
TRandomNumberScenario::TRandomNumberScenario()
    : TScenario("random_number")
{
}
```

Параметр `enableDebugGraph` является опциональным и позволяет проводить [трассировку вызовов](../apphost/interact.md) локального и сетевого графов.

### Макро для регистрации сценария в системе
```c++
HW_REGISTER(MyScenarioName);
```

Это макро должно располагаться в исходном `.cpp`-файле, который имеет признак GLOBAL в соответствующем `ya.make!`.


## Функции настройки сценариев в конструкторе

### Разрешенные SemanticFrames

```c++
void AddSemanticFrame(const TStringBuf frameName);
```

Определение списка разрешенных SemanticFrames, которые обрабатывают сценарий.

Функция опциональна, при отсутствии вызовов `AddSemanticFrame()` фреймворк не будет проверять исходный запрос на наличие данных фреймов.

Если сценарий добавил один или несколько названий семантик фреймов, то все запросы, не содержащие этих фреймов, будут сразу классифицироваться как иррелевантные.

### Таймаут для ScenarioState

```c++
void SetScenarioStateTimeout(std::chrono::seconds timeout);
```

Установка таймаута для получения ScenarioState.

В случае установки ненулевого значения метод `TStorage::GetScenarioState()` может вернуть код ошибки `EScenarioStateResult::Expired`. Более подробно в описании класса [TStorage](tstorage.md).

### NLG-обработчики

```c++
void SetNlgRegistration(TCompiledNlgComponent::TRegisterFunction registerFunction);
```

Регистрация обработчиков NLG для работы функций рендеров.

