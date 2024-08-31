# js

Содержит [JavaScript-блок](../concepts/block-js-ov.md).

Обязательно указание пространства имен http://www.yandex.ru/xscript.

## Содержит {#contains}

[guard](guard.md), [guard-not](guard-not.md), [xpath](xpath.md), [name](name.md) и [param](param.md).

## Содержится в {#contained-in}

корневом элементе (\<page\> или другом) и тегах составных блоков.

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
|| id | Идентификатор блока. | Строка | - ||
|| name | Имя XML-элемента, в котором размещается результат работы блока. | Строка | js ||
|| xmlns | Неймспейс, который будут использоваться в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```xml
<x:js>
  <![CDATA[

    var hello = "Hello world";
    xscript.print(hello);
        
    function sayHello() {
      xscript.xmlprint ("<again>Hello world once more</again>");
    }
              
    sayHello();

    ]]>    
</x:js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [Методы JavaScript-блока](../appendices/block-js-xscript-methods.md)