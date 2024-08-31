# xslt

Содержит путь к файлу перблочного XSL-преобразования. Если type имеет значение `StateArg` или `LocalArg`, путь к файлу преобразования будет взят из переменной контейнера соответствующего типа.

Если переменная, указанная в теге, не определена или ее значение равно пустой строке, то XSL не накладывается.

Имя XSL-наложения определяется до начала работы блока и в ходе его работы изменяться не может. Это связано с тем, что имя перблочного преобразования используется при формировании ключа для [tagged-кеша](../concepts/block-results-caching.md).

## Не содержит других элементов. {#not-contains}

## Содержится в {#contained-in}

тегах любых [блоков](../concepts/block-ov.md).

## Атрибуты {#attrs}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| type | Если указано значение `StateArg` или `LocalArg`, путь к XSL-файлу будет извлекаться из переменной контейнера соответствующего типа. | `StateArg` или `LocalArg` | - ||
|#

## Примеры  {#examples}

```
<mist method="set_state_string">
     <param type="String">xsl</param>
     <param type="String">example.xsl</param> 
</mist> 
<file>
     <xslt type="StateArg">xsl</xslt>
     <method>load</method>
     <param type="String">file.xml</param> 
</file>
```

```
<page xmlns:x="http://www.yandex.ru/xscript">
<x:local>
    <param id="xslt-local" type="String">./simple-stub.xsl</param>
    <param id="xp-local" type="String">head/title</param>
    <root name="toor">
       <file method="load">
           <param type="String">page.xml</param>
           <xslt type="LocalArg">xslt-local</xslt>
           <xpointer type="LocalArg">xp-local</xpointer>
        </file>
    </root>
</x:local>
</page>
```

### Узнайте больше {#learn-more}
* [Перблочное XSL-преобразование](../concepts/per-block-transformation-ov.md)