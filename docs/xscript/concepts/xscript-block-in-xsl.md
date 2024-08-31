# XScript-блок в файле XSL-преобразования

[XScript-блок](block-ov.md) можно поместить в файл основного или перблочного XSL-преобразования. Возможности и синтаксис блоков при этом доступны те же, что и при написании блоков в XML-файле, но: 
- результаты работы блоков не кэшируются;
- вызов метода XScript-блока не может быть асинхронным;
- на блок не может накладываться перблочное XSL-преобразование.

Для описании условия вызова блока можно использовать весь синтаксис XSL .

Теги блоков используются так же, как и в XML, но обязательно должен быть указан префикс пространства имен (namespace) xscript, например:

```
<x:block> 
  <name>Yandex/Validator/Email.id</name> 
  <method>editListX</method> 
  <param type="Request"/> 
  <param type="Auth"/> 
</x:block>
```

Данная функциональность реализована в пространстве имён `xscript`, которое необходимо подключить в XSL-файле.

{% note info %}

Внутри XScript-блока, находящегося в XSL-файле, не следует использовать возможности XSL.

{% endnote %}

### Узнайте больше {#learn-more}
* [Перблочное XSL-преобразование](../concepts/per-block-transformation-ov.md)
* [Основное XSL-преобразование](../concepts/general-transformation-ov.md)
* [http://www.w3.org/TR/REC-xml-names](http://www.w3.org/TR/REC-xml-names)
* [http://www.jclark.com/xml/xmlns.htm](http://www.jclark.com/xml/xmlns.htm)
* [http://www.rol.ru/news/it/helpdesk/xnamsps.htm](http://www.rol.ru/news/it/helpdesk/xnamsps.htm)