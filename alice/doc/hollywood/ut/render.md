# Тестирование рендеров

Тестирование функций рендера позволяет быстро проверить результат работы NLG-генератора и выходных директив при помощи быстрого и компактного вызова:

```c++
UNIT_ASSERT(testData >> TTestRender(&MyRenderFn, RenderArgsProto) >> testResult);
```

Для работы рендера требуется только создание и заполнение `RenderArgsProto`. Данные из `testData` (например, `TRunRequest`) обычно не используются.

После окончания работы рендера можно проверить результат:

* просмотреть содержимое протобафа `testResult.Response`;
* использовать функции-хелперы, напрмер, `testResult.GetText()`, `testResult.ContainsVoice()`.

Из-за вариативности ответов NLG с chooseline нужна аккуратная проверка текстов.

{% cut "Пример с тестами рендера" %}

```c++
// Декларируем testEnvironment
TTestEnvironment testData("my_scenario", "ru-ru");
TTestEnvironment testResult(testData);
// Декларируем и заполняем протобаф с данными для рендеринга
RenderArgsProto myRenderArgs;
myRenderArgs.SetIntValue(123);
myRenderArgs.SetStringValue("TEST");
// Вызываем функцию рендера
UNIT_ASSERT(testData >> TTestRender(&MyRenderFn, myRenderArgs) >> testResult);
// В testResult проверяем корректность ответа
UNIT_ASSERT(!testResult.IsIrrelevant());
UNIT_ASSERT(testResult.ContainsText("Hello, TEST. Value is 123"));
```

{% endcut %}
