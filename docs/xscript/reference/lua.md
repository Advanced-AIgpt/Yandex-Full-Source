# lua

Содержит [Lua-блок](../concepts/block-lua-ov.md) или Lua-код для обработки [мета-информации](../concepts/meta.md) о вызове блока.

## Содержит {#contains}

[guard](guard.md), [guard-not](guard-not.md), [xpath](xpath.md), [name](name.md) и [param](param.md).

## Содержится в {#contained-in}

корневом элементе (\<page\> или другом) - как Lua-блок, или в теге [meta](meta-tag.md) - как Lua-код для обработки мета-информации о вызове блока.

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
|| name | Имя XML-элемента, в котором размещается результат работы блока. | Строка | lua ||
|| xmlns | Неймспейс, который будут использоваться в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```
<lua>
    <![CDATA[
        hello = "Hello world"
        
        print(hello)
        
        function sayHello()
          print ("Hello world again")
        end
              
        sayHello()
    ]]>    
</lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [Методы Lua-блока](../appendices/block-lua-state-methods.md)