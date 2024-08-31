# Методы File-блока

Входные параметры методов File-блока могут конкатенироваться.

Если метод принимает несколько параметров, в первую очередь из списка параметров для конкатенации отбрасываются параметры Tag. Среди оставшихся параметров отбрасываются последние n-1 параметров, где n — количество параметров, которые принимает метод. Остальные параметры конкатенируются.

**Список методов File-блока**:
- [include](block-file-methods.md#include);
- [invoke](block-file-methods.md#invoke);
- [load](block-file-methods.md#load);
- [loadBinary](block-file-methods.md#loadbinary);
- [loadJson](block-file-methods.md#loadJson);
- [loadText](block-file-methods.md#loadtext);
- [test](block-file-methods.md#test).

#### `include` {#include}

Обрабатывает директивы [xi:include](../reference/xi-include.md) в загружаемом документе и вставляет получившийся XML-фрагмент в результирующий документ.

**Входные параметры**: абсолютный или относительный путь к загружаемому XML-документу.

**Пример**:
```
<file>
   <method>include</method>
   <param type="String">/usr/local/www/project/index.xml</param>
</file>
```

#### `invoke` {#invoke}

Обрабатывает директивы `xi:include` и выполняет вызовы XScript-блоков, если они встречаются в загружаемом документе, и вставляет получившийся XML-фрагмент в результирующий XML-документ.

Результат парсинга загружаемого XML-документа кэшируется в XML-кэше в памяти

Для управления временем кэширования результатов работы метода можно использовать метод Lua-блока [setExpireDelta](block-lua-other-methods.md#set-expire-delta).

**Входные параметры**: абсолютный или относительный путь к загружаемому XML-документу.

**Пример**:
```
<file>
   <method>invoke</method>
   <param type="String">/usr/local/www/project/index.xml</param>
</file>
```

#### `load` {#load}

Вставляет указанный XML-документ в результирующий XML-документ.

**Входные параметры**: абсолютный или относительный путь к загружаемому XML-документу.

**Пример**:
```
<file>
   <method>load</method>
   <param type="String">/usr/local/www/project/index.xml</param>
</file>
```

#### `loadBinary` {#loadbinary}

Формирует HTTP-ответ, тело которого состоит из содержимого указанного файла.

В случае успешного выполнения [основное XSL-преобразование](../concepts/general-transformation-ov.md) не накладывается, и блок формирует ответ вида:

```
<success file="/usr/local/www/project/response.xml">1</success>
```

Этот ответ может быть использован при обработке страницы средствами [перблочного XSL-преобразования](../concepts/per-block-transformation-ov.md) и [XPath](../reference/xpath.md).

**Входные параметры**: абсолютный или относительный путь к загружаемому файлу.

**Пример**:

Без обработки элемента `success`:

```
<file>
    <method>loadBinary</method>
    <param type="String">jslib.js</param>
</file>
```

С обработкой элемента `success`:

```
<page>
    <file method="loadBinary">
      <param type="String">logo.png</param>
      <xpath expr="/success" result="load_status"/>
    </file>
    <lua>
      <guard type="StateArg" value="1">load_status</guard>
      xscript.response:setHeader('Content-Type', 'image/png')
    </lua>
</page>

```

#### `loadJson` {#loadJson}

Загружает JSON-данные из указанного файла, [преобразует их в XML](json-to-xml.md) и вставляет результат в тело HTTP-ответа.

**Входные параметры**: абсолютный или относительный путь к загружаемому файлу.

**Пример**:

```
<file>
    <method>loadJson</method>
    <param type="String">data/todays_news.json</param>
</file>
```

#### `loadText` {#loadtext}

Загружает указанный текстовый файл, формирует ответ с корневым тегом `<text>`, телом которого является содержимое файла, к которому применен XML-эскейпинг. XML-эскейпинг подразумевает следующую замену символов в строке:

```
> на &gt;
< на &lt;
& на &amp;
' на &#39;
" на &quot;
```

**Входные параметры**: абсолютный или относительный путь к загружаемому файлу.

**Пример**:

```
<file>
    <method>loadText</method>
    <param type="String">/usr/local/www/project/text.txt</param>
</file>
```

#### `test` {#test}

Проверяет, существует ли файл по указанному пути. Возвращает XML-фрагмент вида:

```
<exist file="/usr/local/www/devel/index.xml" success="1">1</exist>
```

В теге `<exist>` выводится значение "1", если указанный файл существует, и "0", если не существует.

В атрибуте `file` выводится путь к запрашиваемому файлу.

**Входные параметры**: путь к файлу.

**Пример**:
```
<file method="test">
    <param type="String">dps://db/symbols.ent</param>
    <xpath expr="/exist[. = 1]" result="exist_success_1"/>
</file>

<mist method="dumpState"/> 
```

При наличии файла в результате выполнения XPath-выражения `<xpath expr="/exist[. = 1]" result="exist_success_1"/>` в State будет добавлена переменная `exist_success_1`со значением "1", что позволяет использовать её в [guard](../reference/guard.md)-условии перед выполнением другого блока.

### Узнайте больше {#learn-more}
* [File-блок](../concepts/block-file-ov.md)
* [file](../reference/file.md)
