# Функции таблицы xscript

Дополнительные функции Lua, которые можно использовать в блоке, размещены в таблице xscript. Эта таблица содержит как функции, так и вложенные таблицы, также содержащие функции. В настоящий момент в таблицу xscript вложены две таблицы — ycookie и logger.

#### Список функций таблицы xscript: 

- [attachStylesheet](block-lua-other-methods.md#attach-stylesheet);
- [base64decode](block-lua-other-methods.md#base64decode);
- [base64encode](block-lua-other-methods.md#base64encode);
- [crc32](block-lua-other-methods.md#crc32);
- [dateparse](block-lua-other-methods.md#dateparse);
- [domain](block-lua-other-methods.md#other_domain);
- [dropStylesheet](block-lua-other-methods.md#drop-stylesheet);
- [getVHostArg](block-lua-other-methods.md#getvhostarg);
- [md5](block-lua-other-methods.md#md5);
- [print](block-lua-other-methods.md#print);
- [punycodeDomainDecode](block-lua-other-methods.md#punycodeDomainDecode);
- [punycodeDomainEncode](block-lua-other-methods.md#punycodeDomainEncode);
- [setExpireDelta](block-lua-other-methods.md#set-expire-delta);
- [skipNextBlocks](block-lua-other-methods.md#skip-next-blocks);
- [stopBlocks](block-lua-other-methods.md#stop-blocks);
- [strsplit](block-lua-other-methods.md#strsplit);
- [suppressBody](block-lua-other-methods.md#suppress-body);
- [urldecode](block-lua-other-methods.md#urldecode);
- [urlencode](block-lua-other-methods.md#urlencode);
- [xmlescape](block-lua-other-methods.md#xmlescape).

#### Список функций таблицы xscript.ycookie: 

- getValue;
- merge.

#### `attachStylesheet(path)` {#attach-stylesheet}

Задает файл основного XSL-преобразования, путь к которому передан в параметре path.

#### `base64decode(str)` {#base64decode}

Выполняет декодирование строки `str`, закодированной с использованием схемы Base64.

**Пример**:

```
<lua>
    decoded = xscript.base64decode('ZW5jb2RlIG1l')
</lua>
```

#### `base64encode(str)` {#base64encode}

Выполняет кодирование строки `str` с использованием схемы Base64. В передаваемых данных не должно быть символа окончания строки (0х00).

**Пример**:

```
<lua>
    encoded = xscript.base64encode('encode me')
</lua>
```

#### `dateparse(string)` {#dateparse}

Преобразует время `string` в формат unixtime.

`string` - время в формате, использующемся в в HTTP-заголовках _Expires_ и _Set-Cookie_.

**Пример**:

```
<lua>
     time = xscript.dateparse("Fri, 18 Jun 2010 08:58:08 GMT");
</lua>
```

#### `crc32(value)` {#crc32}

Вычисляет контрольную сумму от данных, переданных в качестве параметра.
```
sum = xscript.crc32('Some value');
```

#### `domain(url, level)` {#other_domain}

Выделяет домен уровня level (опциональный параметр) из названия хоста или URL-а.

**Пример**:

```
domain = xscript.domain('http://www.yandex.ru:12345/index.xml')
domain = xscript.domain('http://www.yandex.ru:12345/index.xml', 2)
```

#### `dropStylesheet()` {#drop-stylesheet}

Отменяет основное XSL-преобразование.

#### `getVHostArg(path)` {#getvhostarg}

Позволяет получить значение параметра [конфигурационного файла](config.md) или переменной окружения, имя которой начинается с "XSCRIPT_".

В качестве входного параметра методу передается путь к интересующей настройке, исключая корневой элемент \<xscript\>, или имя переменной окружения.

**Пример**:

```
<x:lua>
    root_domain = xscript.getVHostArg('auth/root-domain')
    env_var = xscript.getVHostArg('XSCRIPT_ENV_VAR')
</x:lua>
```

В приведенном примере выполняется получение параметра конфигурационного файла

```
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

Вычисляет хэш md5 для строки `value`.

Например:

```
sum = xscript.md5('Some value');
```

#### `print()` {#print}

Собирает результаты печати в Lua-блоке в один текстовый блок и оборачивает его тегом `<lua>`.

Если наряду с функцией print используется инструкция [return](../concepts/block-lua-ov.md#return), будет сформирован общий набор узлов, в котором результат работы функции `print` будет представлен в виде текстового узла.

**Пример**:

```
<?xml version="1.0" ?>
<?xml-stylesheet type="text/xsl" href="object.xsl"?>
<page xmlns:x="http://www.yandex.ru/xscript" xmlns:xi="http://www.w3.org/2001/XInclude">
    <x:lua>
        print("Hello")
        print("World!")
    </x:lua>
</page>
```

В результате обработки блока будет сформирован следующий ответ:

```
<?xml version="1.0" encoding="utf-8"?>
<page xmlns:x="http://www.yandex.ru/xscript" xmlns:xi="http://www.w3.org/2001/XInclude">
    <lua>Hello
World!
    </lua>
</page>
```

#### `setExpireDelta(time)` {#set-expire-delta}

Устанавливает время кэширования результатов работы [Local-блока](../concepts/block-local-ov.md) или метода [File-блока](block-file-methods.md) [invoke](block-file-methods.md#invoke).

Принимает на вход время кэширования в секундах.

**Пример**:

```
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
         <lua>xscript.setExpireDelta(60)</lua> 
     </root>
</local>
```

#### `punycodeDomainDecode(domain, encoding)` {#punycodeDomainDecode}

Производит [punycode-декодирование](http://www.wwhois.ru/chto-takoe-punycode.md) доменного имени `domain`. При таком декодировании символы преобразуются из кодировки LDH в символы национальных алфавитов.

Для указания результирующей кодировки используется опциональный параметр `encoding`. По умолчанию предполагается, что результирующая кодировка UTF-8.

Метод предназначен для обработки имен [IDN-доменов](http://ru.wikipedia.org/wiki/IDN).

**Пример:**

```
<lua>
    local decoded = xscript.punycodeDomainDecode('www.xn--d1acpjx3f.xn--p1ai')
    print(decoded)
</lua>
```

В результате обработки блока будет сформирован следующий ответ:

```
<lua>
    www.яндекс.рф
</lua>
```

#### `punycodeDomainEncode(domain, encoding)` {#punycodeDomainEncode}

Производит [punycode-кодирование](http://www.wwhois.ru/chto-takoe-punycode.md) доменного имени `domain`. При таком кодировании символы национальных алфавитов преобразуются в кодировку LDH.

Для указания исходной кодировки доменного имени используется опциональный параметр `encoding`. По умолчанию предполагается, что доменное имя задано с помощью кодировки UTF-8.

Метод предназначен для обработки имен [IDN-доменов](http://ru.wikipedia.org/wiki/IDN).

**Пример:**

```
<lua>
    local encoded = xscript.punycodeDomainEncode('www.яндекс.рф')
    print(encoded)
</lua>
```

В результате обработки блока будет сформирован следующий ответ:

```
<lua>
    www.xn--d1acpjx3f.xn--p1ai
</lua>
```

#### `skipNextBlocks()` {#skip-next-blocks}

XScript не будет выполнять блоки, не запущенные к моменту вызова данного метода. Все уже запущенные блоки закончат свою работу в обычном режиме.

#### `stopBlocks()` {#stop-blocks}

XScript не будет выполнять блоки, не запущенные к моменту вызова данного метода, а работа уже запущенных блоков будет прервана.

#### `strsplit(string, pattern)` {#strsplit}

Разделяет строку `string` разделителем `pattern`. Полученные подстроки помещаются в таблицу, которая возвращается в качестве результата работы метода.

**Пример**:

```
<lua>
    splitted = xscript.strsplit('as,bf,cd,,dgg', ',')
</lua>
```

В результате выполнения блока будет возвращена таблица, заполненная значениями "as", "bf", "cd", "", "dgg".

#### `suppressBody()` {#suppress-body}

В результате вызова данного метода тело HTTP-ответа становится пустым.

#### `urldecode(value, encoding)` {#urldecode}

Декодирует URL-кодированную (URL encoded) строку `value`, а затем переводит из кодировки `encoding` в UTF-8.

#### `urlencode(value, encoding)` {#urlencode}

Переводит строку `value` в кодировку `encoding`, а затем URL-кодирует (URL encode) её, заменяя каждый символ строки на знак процента (%), за которыми следует коды символа в шестнадцатеричном формате.

Например:

```
enc = xscript.urlencode("Привет", “cp1251”)
```

#### `xmlescape(value)` {#xmlescape}

Возвращает копию строки `value`, к которой применен xml-escaping, т.е. выполнена следующая замена символов:

```
> на &gt;
< на &lt;
& на &amp;
' на &#39;
" на &quot;
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)