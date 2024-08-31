# custom-morda

Содержит один блок Custom-morda.

## Содержит {#contains}

[guard](guard.md), [guard-not](guard-not.md), [xpath](xpath.md), [method](method.md) и [param](param.md).

## Содержится в {#contained-in}

корневом элементе (\<page\> или другом).

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
|| method | Имя метода, который должен быть вызван при обработке блока. | Строка | - ||
|| xmlns | Неймспейс, который будут использоваться в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```
<custom-morda>
   <method>save_tune</method>
   <param type="String">block,city_id,count</param>
   <param type="Long">27</param>
</custom-morda>
```

### Узнайте больше {#learn-more}
* [Custom-morda-блок](../concepts/block-custom-morda-ov.md)
* [Методы блока custom-morda](../appendices/block-custom-morda-methods.md)