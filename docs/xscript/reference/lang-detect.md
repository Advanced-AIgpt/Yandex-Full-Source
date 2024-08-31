# lang-detect

Содержит один Lang-detect-блок.

## Содержит {#contains}

[guard](guard.md), [guard-not](guard-not.md), [xpath](xpath.md), [method](method.md) и [param](param.md).

## Содержится в {#contained-in}

тегах любых [блоков](../concepts/block-ov.md).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| guard | Условное выполнение блока.

Содержит имя переменной в контейнере [State](../concepts/state-ov.md), значение которой определяет, будет ли выполен блок.

Использование этого атрибута аналогично использованию в блоке тега [guard](../reference/guard.md) без атрибутов `type` и `value`. | Строка | - ||
|| guard-not | guard-отрицание.

Условное выполнение блока.

Содержит имя переменной в контейнере [State](../concepts/state-ov.md), значение которой определяет, будет ли выполен блок.

Использование этого атрибута аналогично использованию в блоке тега [guard-not](../reference/guard-not.md) без атрибутов `type` и `value`. | Строка | - ||
|| id | Идентификатор блока. | Строка. | - ||
|| xmlns | Неймспейс, который будут использоваться в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```
<x:lang-detect method="find">
  <param type="StateArg" id="parents"/>
  <param type="StateArg">filter</param>
</x:lang-detect>
```

### Узнайте больше {#learn-more}
* [Lang-detect-блок](../concepts/block-lang-detect-ov.md)
* [Методы Lang-detect-блока](../appendices/block-lang-detect-methods.md)