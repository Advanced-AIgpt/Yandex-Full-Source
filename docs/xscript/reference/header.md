# header

- При использовании в элементе [xscript](xscript.md) выставляет HTTP-заголовок ответа на запрос (response header).
    
    Заголовки могут также выставляться в объекте Yandex::Request методами CORBA-объектов.
    
    Если заголовок с одним и тем же именем выставлен и методом CORBA-объекта, и тегом `<header>`, в ответе будет возвращено значение, заданное в `<header>`.
    

- При использовании в элементе [http](http.md) выставляет HTTP-заголовок запроса (request header).
    
    Позволяет определить собственный заголовок и/или изменить существующий. Изменение существующего заголовка возможно в том случае, если в теге http используется атрибут `proxy="yes"`.
    
    В целях безопасности содержимое заголовка
 обрезается по первому разделителю CR ("\r") или LF ("\n").
    
    Содержимое заголовка может быть пустым.
    

Количество тегов `<header>` в обоих случаях может быть произвольным.

## Содержит {#contains}

Данный тег не может содержать других тегов.

## Содержится в {#contained-in}

[add-header](add-header.md), [http](http.md)

## Атрибуты тега при использовании в элементе \<add-headers\> {#attrs-elem}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| name | Обязательный. Имя HTTP-заголовка. | Строка. | - ||
|| value | Обязательный. Содержимое заголовка. | Строка. | - ||
|#

## Атрибуты тега при использовании в элементе \<http\> {#attrs-tag-http}

#|
|| Наименование | Описание | Тип и варианты значения | Значение по умолчанию ||
|| id | Обязательный. Имя HTTP-заголовка. | Строка, удовлетворяющая следующим правилам:

- первый символ принадлежит множеству [a..z],[A..Z];
- второй и последующие символы принадлежат множеству [a..z],[A..Z],[0..9],"-";
- длина строки не превышает 128 символов. | - ||
|| type | Обязательный. Тип параметра, используемого для выставления заголовка. | Приводимый к строковому значению. | - ||
|| default | Необязательный. Содержимое заголовка, используемое по умолчанию. | Строка. | - ||
|#

## Пример использования в теге \<add-headers\> {#attrs-tag}

```
<xscript>
  \<add-headers\>
    <header name="Expires" value="0"/>
    <header name="Cache-Control" value="no-cache, no-store, max-age=0, must-revalidate"/>
    <header name="Pragma" value="no-cache"/>
  </add-headers>
</xscript>
```

## Пример использования в теге \<http\> {#example-http}

В качестве примера рассмотрим XScript-страницу, обращающуюся по HTTP к другой странице, выводящей заголовки входящего запроса с помощью метода [echo_headers](../appendices/block-mist-methods.md#echo_headers). Поскольку данный метод, выводит каждый заголовок дважды, удалим дубликаты с помощью XSL-шаблона.

Страница вывода входящих заголовков - echo.xml:

```
<?xml version="1.0" encoding="UTF-8"?>
<echo>
  <mist>
    <method>echo_headers</method>
  </mist>
</echo>
```

Обращение к `echo.xml`, управление исходящими заголовками -  headers.xml :

```
<?xml-stylesheet type="text/xsl" href="**no_duplicates.xsl**" encoding="UTF-8"?>
<page xmlns:x="http://www.yandex.ru/xscript">

  <x:local proxy="request">
    <root>
      <http method="getHttp">
        <param type="HttpHeader">host</param>
        <param type="String">/echo.xml</param>
      </http>
    </root>
    </x:local>

    <http method="getHttp">
      <param type="HttpHeader">host</param>
      <param type="String">/echo.xml</param>
      <!-- Need some JSON -->
      <header id="ACCEPT" type="String">text/json</header>
      <!-- Custom header -->
      <header id="X-Konkurentam" type="String">Preved!</header>
      <!-- Super IP -->
      <header id="X-Real-IP" type="String">0.0.0.0</header>
      <!-- Some stuff from request -->
      <header id="X-UID" type="UID"/>
      <header id="X-IP" type="ProtocolArg">remote_ip</header>
      <header id="X-User-Region" type="HttpHeader" default="213">X-REGION</header>
      <!-- Trim by CR or LF by -->
      <header id="Security-Check1" type="String">Has to be splitted
        here
      </header> 
      <!-- To be ignored -->
      <header id="EMPTY1" type="String"/>
      <header id="EMPTY2" type="StateArg"/>
    </http>

</page>
```

Удаление дубликатов заголовков - no_duplicates.xsl:

```
<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output  method = "xml"/>

  <xsl:template match="@*|node()">
    <xsl:copy><xsl:apply-templates select="node()"/></xsl:copy>
  </xsl:template>
  <xsl:template match="param"/>
  
</xsl:stylesheet>
```

В результате вызова headers.xml будет сформирован вывод, аналогичный следующему:

```xml
<?xml version="1.0"?>
<page>
  <root>
    <echo>
      <state>
        <ACCEPT>*/*</ACCEPT>
        <ACCEPT-ENCODING>gzip,deflate</ACCEPT-ENCODING>
        <CONNECTION>close</CONNECTION>
        <HOST>mironov.user.graymantle.yandex.ru</HOST>
        <X-FORWARDED-FOR>95.108.175.63</X-FORWARDED-FOR>
        <X-ORIGINAL-URI>/echo.xml</X-ORIGINAL-URI>
        <X-REAL-IP>95.108.175.63</X-REAL-IP>
        <X-REGION>9999</X-REGION>
      </state>
    </echo>
  </root>
  <echo>
    <state>
      <ACCEPT>text/json</ACCEPT>
      <ACCEPT-ENCODING>gzip,deflate</ACCEPT-ENCODING>
      <CONNECTION>close</CONNECTION>
      <HOST>mironov.user.graymantle.yandex.ru</HOST>
      <SECURITY-CHECK1>Has to be splitted</SECURITY-CHECK1>
      <X-FORWARDED-FOR>95.108.175.63</X-FORWARDED-FOR>
      <X-IP>95.108.175.63</X-IP>
      <X-KONKURENTAM>Preved!</X-KONKURENTAM>
      <X-ORIGINAL-URI>/echo.xml</X-ORIGINAL-URI>
      <X-REAL-IP>0.0.0.0</X-REAL-IP>
      <X-REGION>9999</X-REGION>
      <X-USER-REGION>9999</X-USER-REGION>
    </state>
  </echo>
</page>
```

* [Общий процесс обработки запроса](../concepts/request-handling-ov.md)
* [http://www.comptechdoc.org/independent/web/http/reference](http://www.comptechdoc.org/independent/web/http/reference)