# Модуль xscript.uatraits

**Список функций:**

- detect

#### `detect`

Возвращает описание браузера клиента.

**Входные параметры**:

- Значение поля User-Agent http-заголовка

**Примечания**

Метод может быть вызван без передачи параметра, в этом случае будет определен текущий браузер клиента.

#### Пример:

```
<!-- Определение текущего браузера -->
<x:js>
  xscript.print(xscript.request.headers['User-Agent']);
  xscript.print( JSON.stringify( xscript.uatraits.detect() ) );
</x:js>

<!-- Определение браузера по передаваемому параметру -->
<x:js>
  var ua = 'Opera/9.80 (Windows NT 6.1; U; en) Presto/2.10.229 Version/11.61';  
  xscript.print('Query UA: ', ua);
  xscript.print( JSON.stringify( xscript.uatraits.detect(ua) ) );
</x:js></page>
```

В результате обращения из браузера Mozilla Firefox 10.0.1 к приведенной выше странице получим следующий результат:

```xml
<page>
    <!-- Определение текущего браузера -->
    <js>
        Mozilla/5.0 (Windows NT 6.1; WOW64; rv:10.0.1) Gecko/20100101 Firefox/10.0.1
{"BrowserEngine":"Gecko","BrowserEngineVersion":"10.0.1","BrowserName":"Firefox","BrowserVersion":"10.0","OSFamily":"Windows","OSName":"Windows 7","OSVersion":"6.1","isMobile":false,"x64":true}
    </js>
    <!-- Определение браузера по передаваемому параметру -->
    <js>
        Query UA: Opera/9.80 (Windows NT 6.1; U; en) Presto/2.10.229 Version/11.61
{"BrowserEngine":"Presto","BrowserEngineVersion":"2.10.229","BrowserName":"Opera","BrowserVersion":"11.61","OSFamily":"Windows","OSName":"Windows 7","OSVersion":"6.1","isMobile":false}
    </js>
</page>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)