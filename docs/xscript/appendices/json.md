# Поддержка JSON

XScript содержит внутренние компоненты, позволяющие работать с данными в формате JSON. Компоненты реализованы в модуле [xscript-json.so](../concepts/modules.md#xscript-json). После подключения модуля становятся доступными следующие возможности:

- Загрузка данных в формате JSON и преобразование их в формат XML. JSON-данные могут загружаться как из файла (в [File-блоке](../concepts/block-file-ov.md)), так и по протоколу HTTP (с помощью [HTTP-блока](../concepts/block-http-ov.md)).
- Преобразование XML-элементов в формат JSON. Осуществляется средствами XSL: путем задания метода вывода в XSL-шаблоне (`<xsl:output method="x:json" .../>`) или с помощью XSL-функций [x:json-stringify](xslt-functions.md#json-stringify) и [x:js-stringify](xslt-functions.md#js-stringify).

В процессе преобразования не участвуют никакие XML-атрибуты за исключением атрибутов `type` и `name`. Последние имеют специальное значение. Атрибут type используется для явного указания типа данных, содержащихся в XML-элементе. С помощью атрибута `name` можно в явном виде задать ключ, по которому преобразованный XML-элемент размещается в JSON-структуре.

Также игнорируются префиксы пространств имен. При преобразовании [XML→JSON](xml-to-json.md) данные о пространствах имен не попадают в JSON-выдачу, при преобразовании [JSON→XML](json-to-xml.md) в результирующем XML пространства имен использоваться не будут.

Преобразования XML→JSON и JSON→XML симметричны. Это означает, что если не учитывать XML-атрибуты и пространства имен, то последовательное преобразование XML→JSON→XML или JSON→XML→JSON оставит исходную структуру и данные неизменными с точностью до порядка следования элементов.

**Пример**

В данном примере производится обращение по HTTP к геокодеру Яндекс-карт, ответ запрашивается в формате JSON.

HTTP-блок анализирует заголовки ответа, определяет, что ответ возвращается в формате JSON, преобразует полученную структуру в XML и включает ее в XScript-страницу.

Затем накладывается XSL-преобразование, удаляющее из сформированного на предыдущем этапе XML-файла атрибуты `type`, появившиеся во время преобразования JSON→XML.

XScript-страница, запрашивающая результат геокодирования адреса <q>Москва, Новый Арбат 24</q> в формате JSON:

```
<?xml-stylesheet href="**remove_attr.xsl**" type="text/xsl"?>
<page xmlns:x="http://www.yandex.ru/xscript">

<http proxy="yes">
  <method>getHttp</method>
  <param type="String">http://geocode-maps.yandex.ru/1.x/?format=json</param>
  <query-param id="geocode" type="String">Москва, Новый Арбат 24</query-param>
  <query-param id="key" type="String">AKdPv0kBAAAAMh0COgQARX5mWeIyJJyFywj6QyCETlfCy4oAAAAAAAAAAACe3yn6vAxynDngJcllGV4kIE_MzA==</query-param>
</http> 

</page>
```

XSL-шаблон **remove_attr.xsl**, удаляющий атрибут `type`:

```
<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml" encoding="utf-8" indent="yes"/>

<xsl:template match="node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>

<xsl:template match="@type"/>

</xsl:stylesheet>
```

Полученный результат с точностью до порядка следования XML-элементов совпадает с результатом геокодирования, запрошенным в формате XML: 
```
http://geocode-maps.yandex.ru/1.x/?geocode=Москва, Новый Арбат 24&key=AKdPv0kBAAAAMh0COgQARX5mWeIyJJyFywj6QyCETlfCy4oAAAAAAAAAAACe3yn6vAxynDngJcllGV4kIE_MzA==
```

### Узнайте больше {#learn-more}
* [JSON→XML](../appendices/json-to-xml.md)
* [XML→JSON](../appendices/xml-to-json.md)