# Атрибут xpointer

Атрибут _xpointer_ позволяет накладывать XPointer-выражения на результат работы любого блока для выборки из XML части узлов.

В качестве значения атрибута `xpointer` может выступать любое XPath-выражение.

Если в теге блока присутствует атрибут _xmlns_, при обработке XPointer-выражения будет использоваться неймспейс, указанный в этом атрибуте.

XPointer может накладываться на результаты работы блока, вызванного из [основного или перблочного XSL-преобразования](../concepts/xscript-block-in-xsl.md).

Существует возможность использовать в качестве выражения XPointer значение переменной из контейнера [State](../concepts/state-ov.md). Для этого применяется тег [\<xpointer\>](../reference/xpointer-tag.md).

В случае, если веб-сервер запущен на порту, на котором не накладывается ни основной XSL, ни перблочные преобразования, XPointer на результат работы блока накладываться не будет. Также XPointer не будет накладываться, если в результате своей работы блок вернул ошибку.

{% note warning %}

**Отключить вывод блока можно при помощи выражения `xpointer="/.."`**. При этом XPointer фактически не накладывается, поэтому отключение вывода блока производится быстро.

{% endnote %}

## Примеры {#examples}

Допустим, нужно наложить XPointer на результат работы следующего file-блока:
```
<?xml version="1.0" encoding="UTF-8"?>
  <page>
     <file xpointer="//BBB">
       <method>include</method>
       <param type="String">include.xml</param>
     </file>
</page>
```

Файл `include.xml`, подключаемый в этом блоке, имеет следующую структуру:
```
<?xml version="1.0" encoding="UTF-8"?>
<AAA>
     <BBB/>
     <CCC/>
     <BBB/>
     <DDD>
          <BBB/>
     </DDD>
     <CCC>
          <DDD>
               <BBB/>
               <BBB/>
          </DDD>
     </CCC>
</AAA>
```

В результате выполнения блока и наложения xpointer XScript вернет XML вида
```
<page>
        <BBB/>
        <BBB/>
        <BBB/>
        <BBB/>
        <BBB/>
</page>
```

В следующем примере при помощи XPointer-выражения из результата работы блока _удаляется корневой XML-узел_.
```
<block xmlns:dd="http://www.yandex.ru/dd" xpointer="/*/node()">
      <name>Yandex/Validator/Email.id</name>
      <method>editListX</method>
      <param type="Request"/>
      <param type="LiteAuth"/>
</block>
```

При работе с версиями модуля xscript-standard до 5.53–2 не рекомендуется использовать сигнатуру `xpointer="xpointer(/*/node())"` вместо описанной выше, так как это приведет к возникновению ошибок при работе с новыми версиями _libxml2_.

### Узнайте больше {#learn-more}
* [XML Pointer Language (XPointer) Version 1.0](http://www.w3.org/TR/WD-xptr)
* [xpath](../reference/xpath.md)
* [xpointer](../reference/xpointer-tag.md)
