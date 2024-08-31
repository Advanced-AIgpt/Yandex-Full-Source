# local-program

Содержит один блок [Local-program](../concepts/block-local-program-ov.md).

Использование пространства имён _http://www.yandex.ru/xscript_ обязательно.

## Содержит {#contains}

[param](param.md) (необязательно).

## Содержится в {#contained-in}

корневом элементе (\<page\> или другом).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| guard | Условное выполнение блока.<br/><br/>Содержит имя переменной в контейнере [State](../concepts/state-ov.md), значение которой определяет, будет ли выполен блок.<br/><br/>Использование этого атрибута аналогично использованию в блоке тега [guard](../reference/guard.md) без атрибутов `type` и `value`. | Строка | - ||
|| guard-not | guard-отрицание.<br/><br/>Условное выполнение блока.<br/><br/>Содержит имя переменной в контейнере [State](../concepts/state-ov.md), значение которой определяет, будет ли выполен блок.<br/><br/>Использование этого атрибута аналогично использованию в блоке тега [guard-not](../reference/guard-not.md) без атрибутов `type` и `value`. | Строка | - ||
|| id | Идентификатор блока. | Строка | - ||
|| method | Имя метода, который должен быть вызван при обработке блока. | Строка.<br/><br/>Единственно возможное значение check. | - ||
|| xmlns | Пространство имен корневого элемента ответа блока. Используется в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```
<page xmlns:x="http://www.yandex.ru/xscript">
    <x:local-program method="check">
        <param id="ip_to_сheck" type="String">217.195.64.1</param>
    </x:local-program>
</page>
```

### Узнайте больше {#learn-more}
* [Local-program-блок](../concepts/block-local-program-ov.md)