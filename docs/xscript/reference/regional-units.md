# regional-units-block

## Содержит {#contains}

[guard](guard.md), [guard-not](guard-not.md), [xpath](xpath.md), [method](method.md) и [param](param.md).

## Содержится в {#contained-in}

тегах любых [блоков](../concepts/block-ov.md).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| elapsed-time | Если данному атрибуту присвоено значение "yes", в корневой элемент результата работы блока будет добавлен атрибут elapsed-time со значением, указывающим время работы блока в секундах. | "yes" или "no" | no ||
|| guard | Условное выполнение блока.

Содержит имя переменной в контейнере [State](../concepts/state-ov.md), значение которой определяет, будет ли выполен блок.

Использование этого атрибута аналогично использованию в блоке тега [guard](../reference/guard.md) без атрибутов `type` и `value`. | Строка | - ||
|| guard-not | guard-отрицание.

Условное выполнение блока.

Содержит имя переменной в контейнере [State](../concepts/state-ov.md), значение которой определяет, будет ли выполен блок.

Использование этого атрибута аналогично использованию в блоке тега [guard-not](../reference/guard-not.md) без атрибутов `type` и `value`. | Строка | - ||
|| id | Идентификатор блока. | Строка | - ||
|| method | Имя метода, который должен быть вызван при обработке блока. | Строка | - || 
|| threaded | Если данному атрибуту присвоено значение "yes", блок обрабатывается асинхронно (в отдельном потоке). Значение "no" имеет смысл устанавливать в случае, когда необходимо выключить асинхронность обработки конкретного запроса при установленном атрибуте [all-threaded](../reference/xscript.md#all-treaded) ="yes". | "yes" или "no" | no ||
|| timeout | Таймаут выполнения блока (в миллисекундах). По умолчанию этот таймаут равен 5000 миллисекундам (5 сек.). | Целое число. | - ||
|| xmlns | Неймспейс, который будут использоваться в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```
<regional-units-block>
    <method>transform</method>
    <param type="String">US</param>
    <param type="String">distance</param>
    <param type="String">1700</param>
</regional-units-block>
```

### Узнайте больше {#learn-more}
* [Regional-units-блок](../concepts/block-regional-units-ov.md)
* [Методы Regional-units-блока](../appendices/block-regional-units-methods.md)