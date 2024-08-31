# Semantic Frames

Сценарии Голливуда могут получать данные семантических фреймов одним из двух способов:
* **Рекомендуется.** Через `TypedSemanticFrames` (TSF), подготовленные в [Megamind](../../megamind/index.md).
* Через `NAlice::NHollywoodFw::TFrame`, который преобразовывает протобаф в структуру и перекладывает данные в протобаф сцены. Этот способ позволяет:
  * получить данные для тех сценариев, которые опираются на обычные SF, а не на TSF;
  * получить прямой доступ к протобафам `TInput` для самостоятельного инспекта полей.

## TSF

Протобаф с данными из семантического фрейма подготовлен заранее. Получите его при помощи метода класса `TRequest`:

```cpp
template <class T>
bool TRequest::TInput::FindTSF(const TStringBuf frameName, T& proto) const;
```

Действия метода: 
* Если во входных данных найден соответствующий TSF, то заполнит ваш экземпляр протобафа `T`.
* Если не найден фрейм с указанным именем `frameName` и содержащий указанный TSF, то метод вернет `false`.

Если Megamind прислал несколько фреймов с одинаковым именем:
* **но разными TSF**, то будет выбран именно тот фрейм, который содержит TSF нужного типа;
* **и одинаковыми TSF**, то метод вернет первый попавшийся фрейм с TSF нужного типа.

### Обработка пришедших параметров

Если диспетчеру не требуется дополнительная обработка пришедших параметров, то диспетчер может передать указанный протобаф Megamind сразу как аргумент для выбора сцены. 

Если обработка требуется, то диспетчер может сформировать для обработчика сцены новый протобаф с полученным TSF в качестве первого аргумента.

```cpp
TRetScene MyScenario::Dispatch(const TRunRequest& runRequest, const TStorage& storage, const TSource& source) const {
    ...
    TGetTimeSemanticFrame tsf;

    if (!runRequest.GetInput().FindTfs("time_frame", tsf)) {
        return TRetValue::RenderIrrelevant("my_scenario_nlg", "my_scenario_error_phrase");
    }
    // Работаем с заполненным tsf
    ...

```

## TFrame

Этот метод применим в том случае, если у вас нет TSF и параметры SF вы получаете напрямую.

Для преобразования SF в C++ структуру:
1. Создайте структуру-наследника класса `NAlice::NHollywoodFw::TFrame`.
2. Опишите поля структуры при помощи объектов `NAlice::NHollywoodFw::TXxxSlot`: 
   * `TSlot` — обязательное значение, при его отсутствии будет подставлено значение по умолчанию.
   * `TOptionalSlot` содержит `TMaybe<>`.
   * `TArraySlot` содержит массив элементов, возможно, пустой.
   * `TEnumSlot` с автоматическим преобразованием `string->enum` или `string->int`.

В качестве шаблонного параметра `TSlot`, `TOptionalSlot` и `TArraySlot` могут принимать все стандартные типы `bool, [u]i(32|64)`, `float` или `TString`.

{% cut "Пример" %}

Семантический фрейм имеет поля `int geo_id`, повторяющееся поле `float weight` и опциональное поле `string where`. 

Создайте структуру для получения этих полей из Semantic Frame следующего вида:

```cpp
struct TMyFrame : public TFrame {
    TMyFrame(const TRequest::TInput& input);

    TSlot<i32> GeoId;
    TArraySlot<float> Weight;
    TOptionalSlot<TString> Where;
};
```

В конструкторе `TMyFrame` инициализируйте родительский класс `TFrame` и все поля-члены структуры:

```cpp
TMyFrame::TMyFrame(const TRequest::TInput& input)
    : TFrame(input, "my_sf_name")
    , GeoId(this, "geo_id", INVALID_GEOID)
    , Weight(this, "weight")
    , Where(this, "where")
{
}
```

Добавьте в основной код диспетчера:

```cpp
TRetScene MyScenario::Dispatch(const TRunRequest& runRequest, const TStorage& storage, const TSource& source) const {
    ...
    TMyFrame myFrame(request.Input())
    if (myFrame.Defined()) {
        // Работаем с SF
        myFrame.GeoId.Value // тип i32
        myFrame.Weight.Value // тип TArray<float>
        myFrame.Where.Value // тип TMaybe<TString>
        ...
    }
```
{% endcut %}

### Поддержка string to enum маппера

Класс `TEnumSlot` позволяет сохранить в структуре `TFrame` значение, автоматически сконвертированное из строкового поля.
Класс `TEnumSlot` имеет следующий конструктор:

```cpp
TEnumSlot(TFrame* owner, TStringBuf slotName, const TMap<TStringBuf, E>& mapper)
```

В качестве последнего параметра передается мапа из строковых значений поля семантического фрейма и соответствующего темплейтного значения `E` (обычно это `enum class`, но можно использовать и другие типы).
Переменная `mapper` обязана содержать хотя бы одно значение!

{% cut "Пример" %}

Пусть у нас есть слот с именем `tense` в [Гранете](../../nlu/granet/index.md), который представлен следующим образом:

```
    %type string
    %value "past"
    было
    %value "future"
    будет
```

Заводим для него соответствующим enum из трех значений: `past`, `future` и `default` и отправляем этот слот в процессор:

```cpp

enum class Tense {
   Past,
   Future,
   Default
};

...
   TEnumSlot<Tense> TenseValue;
...
TMyFrame::TMyFrame(
    ...
    TenseValue(this, "tense", {{"past", Tense::Past},
                               {"future", Tense::Future},
                               {"", Tense::Default}});
```

Если в качестве одного из полей в списке использовать пустую строку (`""`), то все неизвестные или отсутствующие поля будут сматчены на ее значение `Tense::Default`.

Если последняя строчка маппера содержит непустое строковое значение, то при отсутствии этого ключа в Semantic Frame будет брошено исключение `HW_ERROR()`.

{% endcut %}

### Поддержка sys.date, sys.time, sys.datetime

Эти сущности Гранета поддержаны с помощью библиотеки [sys_datetime](https://a.yandex-team.ru/arc_vcs/alice/library/sys_datetime). Подробнее о библиотеке в разделе [Стандартные библиотеки Алисы](../../alice/handlers/sys_datetime.md).

Реализация доступна для слотов `TOptionalSlot` и `TArraySlot`:

```cpp
    TOptionalSlot<TSysDatetimeParser> SingleDateTimeParserObject;
    TArraySlot<TSysDatetimeParser>    MultipleDateTimeParserObjects;
```