# Методы uatraits-блока

**Список методов блока**:
- detect.

#### `detect`

Возвращает описание браузера клиента.

**Входные параметры**:

- Значение поля User-Agent http-заголовка

**Примечания**

Метод может быть вызван без передачи параметра, в этом случае будет определен текущий браузер клиента.

#### Пример:

```
<!-- Определение текущего браузера -->
<x:uatraits method="detect"/>

<!-- Определение браузера по передаваемому параметру -->
<x:uatraits>
    <method>detect</method>
    <param type="String">Opera/9.80 (Windows NT 6.1; U; en) Presto/2.10.229 Version/11.61</param>
</x:uatraits>
```

В результате обращения из браузера Mozilla Firefox 10.0.1 к приведенной выше странице получим следующий результат:

```xml
<page>
    <uatraits>
        <BrowserEngine>Gecko</BrowserEngine>
        <BrowserEngineVersion>10.0.1</BrowserEngineVersion>
        <BrowserName>Firefox</BrowserName>
        <BrowserVersion>10.0</BrowserVersion>
        <OSFamily>Windows</OSFamily>
        <OSName>Windows 7</OSName>
        <OSVersion>6.1</OSVersion>
        <isMobile>false</isMobile>
        <x64>true</x64>
    </uatraits>
    <uatraits>
        <BrowserEngine>Presto</BrowserEngine>
        <BrowserEngineVersion>2.10.229</BrowserEngineVersion>
        <BrowserName>Opera</BrowserName>
        <BrowserVersion>11.61</BrowserVersion>
        <OSFamily>Windows</OSFamily>
        <OSName>Windows 7</OSName>
        <OSVersion>6.1</OSVersion>
        <isMobile>false</isMobile>
    </uatraits>
</page>
```

### Узнайте больше {#learn-more}
* [uatraits](../reference/uatraits.md)
* [Uatraits-блок](../concepts/block-xscript-uatraits-ov.md)