# local

Содержит один [Local-блок](../concepts/block-local-ov.md).

При использовании Local-блока необходимо указывать в теге \<local\> пространство имен _http://www.yandex.ru/xscript_.

```
<page xmlns:x="http://www.yandex.ru/xscript">
     <x:local>
         ...
     </x:local>
</page>
```

## Содержит {#contains}

[guard](guard.md), [guard-not](guard-not.md), [xpath](xpath.md), [\<root\>](root.md) и [param](param.md).

## Содержится в {#contained-in}

корневом элементе (\<page\> или другом).

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
|| proxy | Если атрибуту присвоено значение <q>yes</q>, блок [имеет доступ к родительским объектам](../concepts/block-local-ov.md#parent_context).

Если атрибут имеет значение <q>request</q>, блок получает [доступ к родительскому объекту Request](../concepts/block-local-ov.md#request_access). | "yes", "request" или "no" | no ||
|| tag | Время кэширования результата работы блока. Может также иметь значение "yes", что означает неопределенное время кэширования (см. [Кэширование результатов работы XScript-блока](../concepts/block-results-caching.md)). | Секунды | - ||
|| threaded | Если данному атрибуту присвоено значение "yes", блок обрабатывается асинхронно (в отдельном потоке). Значение "no" имеет смысл устанавливать в случае, когда необходимо выключить асинхронность обработки конкретного запроса при установленном атрибуте [all-threaded](../reference/xscript.md#all-treaded) ="yes". | "yes" или "no" | no ||
|| timeout | Таймаут выполнения блока (в миллисекундах). По умолчанию этот таймаут равен 5000 миллисекундам (5 сек.). | Целое число. | - ||
|| xmlns | Пространство имен корневого элемента ответа [Local-блока](../concepts/block-local-ov.md). Используется в XPath-выражении при отработке тега [\<xpath\>](../reference/xpath.md) и атрибута xpointer. | Строка | - ||
|| xpointer | [XPointer](../appendices/xpointer.md)-выражение, накладывающееся на результат работы блока. | Строка. Например, ""`//BBB`"" | - ||
|| xslt | Относительный или абсолютный путь к XSL-файлу перблочного преобразования. | Строка. Например, "`/usr/local/www/common/ docbook/docbook_validator.xsl`" | - ||
|#

## Пример {#example}

```
<page xmlns:x="http://www.yandex.ru/xscript">
    <mist method="set_state_long">
        <param type="String">long_var</param>
        <param type="Long">100</param>
    </mist>
    <x:local>
        <param id="name" type="String">my_name</param>
        <param id="var" type="StateArg">long_var</param> 
        <root>
            <mist method="set_state_string">
                <param type="String">local_name</param>
                <param type="LocalArg">name</param>
            </mist>
            <mist method="dumpState"/>
        </root>
    </x:local>
</page>
```

### Узнайте больше {#learn-more}
* [Local-блок](../concepts/block-local-ov.md)