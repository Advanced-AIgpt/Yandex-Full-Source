# Функции пространства имен xscript

Набор функций общего назначения, реализованный в XScript для использования в JavaScript-блоке.

#### Список функций: 

- [attachStylesheet](block-js-xscript-methods.md#attach-stylesheet);
- [base64decode](block-js-xscript-methods.md#base64decode);
- [base64encode](block-js-xscript-methods.md#base64encode);
- [crc32](#crc32);
- [dateparse](block-js-xscript-methods.md#dateparse);
- [domain](block-js-xscript-methods.md#other_domain);
- [dropStylesheet](block-js-xscript-methods.md#drop-stylesheet);
- [getVHostArg](block-js-xscript-methods.md#getvhostarg);
- [md5](block-js-xscript-methods.md#md5);
- [print](block-js-xscript-methods.md#print);
- [punycodeDomainDecode](block-js-xscript-methods.md#punycodeDomainDecode);
- [punycodeDomainEncode](block-js-xscript-methods.md#punycodeDomainEncode);
- [setExpireDelta](block-js-xscript-methods.md#set-expire-delta);
- [stopBlocks](block-js-xscript-methods.md#stop-blocks);
- [suppressBody](block-js-xscript-methods.md#suppress-body);
- [transformToString](#transformtostring);
- [xmlescape](block-js-xscript-methods.md#xmlescape);
- [xmlprint](block-js-xscript-methods.md#xmlprint);
- [xsltProcess](#xsltprocess).

#### `attachStylesheet(path)` {#attach-stylesheet}
Задает файл основного XSL-преобразования, путь к которому передан в параметре `path`.

#### `base64decode(str)` {#base64decode}

Выполняет декодирование строки `str`, закодированной с использованием схемы Base64.

**Пример**:

```xml
<x:js>
  var decoded = xscript.base64decode('ZW5jb2RlIG1l');
</x:js>
```

#### `base64encode(str)` {#base64encode}

Выполняет кодирование строки `str` с использованием схемы Base64. В передаваемых данных не должно быть символа окончания строки (0x00).

**Пример**:

```xml
<x:js>
  var encoded = xscript.base64encode('encode me');
</x:js>
```

#### `crc32(data)` {#crc32}
Вычисляет контрольную сумму от данных, переданных в качестве параметра.

**Пример:**
```
<x:js>
  var crd_sum_str=(xscript.crc32("Hello JS")).toString(16);
  xscript.print(crc_sum_str);
</x:js>
```

#### `dateparse(string)` {#dateparse}

Преобразует время `string` в формат Unix time.

`string` - время в формате, использующемся в в HTTP-заголовках _Expires_ и _Set-Cookie_.

**Пример**:

```xml
<x:js>
     var time = xscript.dateparse("Fri, 18 Jun 2010 08:58:08 GMT");
</x:js>
```

#### `domain(url, level)` {#other_domain}

Выделяет домен уровня `level` (опциональный параметр) из заданного URL.

**Пример**:

```xml
<x:js>
  var domain = xscript.domain('http://www.yandex.ru:12345/index.xml');
  var domain = xscript.domain('http://www.yandex.ru:12345/index.xml', 2);
</x:js>
```

#### `dropStylesheet()` {#drop-stylesheet}

Отменяет основное XSL-преобразование.

#### `getVHostArg(path)` {#getvhostarg}

Позволяет получить значение параметра [конфигурационного файла](config.md) или переменной окружения, имя которой начинается с "XSCRIPT_".

В качестве входного параметра методу передается путь к интересующей настройке, исключая корневой элемент `<xscript>`, или имя переменной окружения.

**Пример**:

```xml
<x:js>
    var root_domain = xscript.getVHostArg('auth/root-domain');
    var env_var = xscript.getVHostArg('XSCRIPT_ENV_VAR');
</x:js>
```

В приведенном примере выполняется получение параметра конфигурационного файла

```xml
<xscript>
  ...
  <auth>
    <root-domain>ya.ru</root-domain>
  </auth>
  ...
</xscript
```

а также переменной окружения `XSCRIPT_ENV_VAR`.

#### `md5(value)` {#md5}

Вычисляет хэш MD5 для строки `value`.

Например:

```xml
<x:js>
  var hash = xscript.md5('Some value');
<x:js>
```

#### `print(text)` {#print}

Во время вызова функции, ее аргумент записывается в поток. В конце исполнения блока этот поток направляются в результирующий документ в виде текстового узла. Все специальные символы экранируются.

**Пример**:

```xml
<?xml version="1.0" ?>
<page xmlns:x="http://www.yandex.ru/xscript">
  <x:js>
    xscript.print("Hello");
    xscript.print("World!");
  </x:js>
</page>
```

В результате обработки блока будет сформирован следующий ответ:

```xml
<?xml version="1.0" encoding="utf-8"?>
<page xmlns:x="http://www.yandex.ru/xscript">
  <js>Hello
World!</js>
</page>
```

#### `setExpireDelta(time)` {#set-expire-delta}

Устанавливает время кэширования результатов работы [Local-блока](../concepts/block-local-ov.md) или метода [File-блока](block-file-methods.md) [invoke](block-file-methods.md#invoke).

Принимает на вход время кэширования в секундах.

**Пример**:

```xml
<local proxy="yes" tag="yes">
  <param id="name" type="String">my_name</param>
    <param id="var" type="StateArg">long_var</param>
      <root name="page">
        <mist method="set_state_string">
          <param type="String">local_name</param>
          <param type="LocalArg">name</param>
        </mist>
        <mist method="dumpState"/>
        <!-- Результаты работы Local-блока будут закешированы на 60 сек -->
        **<x:js>xscript.setExpireDelta(60)</x:js> **
      </root>
</local>
```

#### `punycodeDomainDecode(domain, encoding)` {#punycodeDomainDecode}

Производит [punycode-декодирование](http://www.wwhois.ru/chto-takoe-punycode.md) доменного имени `domain`. При таком декодировании символы преобразуются из кодировки LDH в символы национальных алфавитов.

Для указания результирующей кодировки используется опциональный параметр `encoding`. По умолчанию предполагается, что результирующая кодировка UTF-8.

Метод предназначен для обработки имен [IDN-доменов](http://ru.wikipedia.org/wiki/IDN).

**Пример:**

```xml
<x:js>
  var punyDecoded = xscript.punycodeDomainDecode('www.xn--d1acpjx3f.xn--p1ai');
  xscript.print(punyDecoded);
</x:js>
```

В результате обработки блока будет сформирован следующий ответ:

```xml
<js>www.яндекс.рф</js>
```

#### `punycodeDomainEncode(domain, encoding)` {#punycodeDomainEncode}

Производит [punycode-кодирование](http://www.wwhois.ru/chto-takoe-punycode.md) доменного имени `domain`. При таком кодировании символы национальных алфавитов преобразуются в кодировку LDH.

Для указания исходной кодировки доменного имени используется опциональный параметр `encoding`. По умолчанию предполагается, что доменное имя задано с помощью кодировки UTF-8.

Метод предназначен для обработки имен [IDN-доменов](http://ru.wikipedia.org/wiki/IDN).

**Пример:**

```xml
<x:js>
  var punyEncoded = xscript.punycodeDomainEncode('www.яндекс.рф');
  xscript.print(punyEncoded);
</x:js>
```

В результате обработки блока будет сформирован следующий ответ:

```xml
<js>www.xn--d1acpjx3f.xn--p1ai</js>
```

#### `stopBlocks()` {#stop-blocks}

Запрещает XScript выполнять блоки, не запущенные к моменту вызова данного метода. Работа уже запущенных блоков будет прервана.

#### `suppressBody()` {#suppress-body}

В результате вызова данного метода тело HTTP-ответа становится пустым.

#### `transformToString(xml_text, xsl_file)` {#transformtostring}

Возвращает текстовое представление применения `xsl_file` на `xml_text`. Аналог функции [xsltProcess()](#xsltprocess). Отличие заключается в корректном применении инструкции следующего вида `<xsl:output method=МММ />` из `xsl_file`, где `МММ := { xml | text | html }`.

#### `xmlescape(value)` {#xmlescape}

Возвращает копию строки `value`, к которой применен xml-escaping, т.е. выполнена следующая замена символов:

```xml
> на &gt;
< на &lt;
& на &amp;
' на &#39;
" на &quot;
```

#### `xmlprint(text)`{#xmlprint}

Во время вызова функции, ее аргумент записывается в поток. В конце исполнения блока этот поток сериализуется в XML, валидируется и направляется в результирующий документ. Поток `xmlprint` выводится после потока `[print](block-js-xscript-methods.md#print)`.

**Пример**:

```xml
<?xml version="1.0" encoding="utf-8"?>
<page xmlns:x="http://www.yandex.ru/xscript">
<x:js>
<![CDATA[
  xscript.xmlprint('<hello>Hello JS-world!</hello>');
]]>
</x:js>
```

В результате обработки блока будет сформирован следующий ответ:

```xml
<?xml version="1.0" encoding="utf-8"?>
<page xmlns:x="http://www.yandex.ru/xscript">
  <js><hello>Hello JS-world!</hello></js>
</page>
```

#### `xsltProcess(xml_text, xsl_file)` {#xsltprocess}
Возвращает текстовое представление применения `xsl_file`-преобразования на `xml_text`.

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)
