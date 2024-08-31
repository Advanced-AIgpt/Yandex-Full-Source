# Методы Mist-блока

**Список методов Mist-блока**:
- [attach_stylesheet](block-mist-methods.md#attach_stylesheet);
- [drop_state](block-mist-methods.md#drop_state);
- [dump_state](block-mist-methods.md#dump_state);
- [echo_cookies](block-mist-methods.md#echo_cookies);
- [echo_headers](block-mist-methods.md#echo_headers);
- [echo_local_args](block-mist-methods.md#echo_local_args);
- [echo_protocol](block-mist-methods.md#echo_protocol);
- [echo_query](block-mist-methods.md#echo_query);
- [echo_request](block-mist-methods.md#echo_request);
- [erase_state](block-mist-methods.md#erase_state);
- [location](block-mist-methods.md#location);
- [set_state_by_cookies](block-mist-methods.md#set_state_by_cookie);
- [set_state_by_date](block-mist-methods.md#set_state_by_date);
- [set_state_by_headers](block-mist-methods.md#set_state_by_headers);
- [set_state_by_key](block-mist-methods.md#set_state_by_key);
- [set_state_by_keys](block-mist-methods.md#set_state_by_keys);
- [set_state_by_local_args](block-mist-methods.md#set_state_by_local_args);
- [set_state_by_protocol](block-mist-methods.md#set_state_by_protocol);
- [set_state_by_query](block-mist-methods.md#set_state_by_query);
- [set_state_by_request](block-mist-methods.md#set_state_by_request);
- [set_state_by_request_urlencoded](block-mist-methods.md#set_state_by_request_urlencoded);
- [set_state_concat_string](block-mist-methods.md#set_state_concat_string);
- [set_state_defined](block-mist-methods.md#set_state_defined);
- [set_state_domain](block-mist-methods.md#set_state_domain);
- [set_state_double](block-mist-methods.md#set_state_double);
- [set_state_long](block-mist-methods.md#set_state_long);
- [set_state_longlong](block-mist-methods.md#set_state_longlong);
- [set_state_join_string](block-mist-methods.md#set_state_join_string);
- [set_state_random](block-mist-methods.md#set_state_random);
- [set_state_split_string](block-mist-methods.md#set_state_split_string);
- [set_state_string](block-mist-methods.md#set_state_string);
- [set_state_urldecode](block-mist-methods.md#set_state_urldecode);
- [set_state_urlencode](block-mist-methods.md#set_state_urlencode);
- [set_state_xmlescape](block-mist-methods.md#set_state_xmlescape);
- [set_status](block-mist-methods.md#set_status).

#### `attach_stylesheet` (`attachStylesheet`) {#attach_stylesheet}

Задает файл основного XSL-преобразования.

**Входные параметры**: `path` - относительный или абсолютный путь к XSL-файлу.

**Пример**:

В приведенном ниже примере с помощью [guard](../reference/guard.md)-условия проверяется, возникла ли в ходе обработки запроса ошибка, и если она возникла, на страницу накладывается `/usr/local/www/common/404/404.xsl`.

```
<mist method="attachStylesheet" guard="error" xpointer="/..">
    <param type="String">/usr/local/www/common/404/404.xsl</param>
</mist>
```

#### `drop_state` (`dropState`) {#drop_state}

Удаляет указанные переменные или все переменные из контейнера State.

**Входные параметры**: `prefix` (опциональный параметр) - префикс имен переменных, которые должны быть удалены из State. Если данный параметр не указан, контейнер State очищается полностью.

**Пример**:

```
<mist method="drop_state" />
```

#### `dump_state` (`dumpState`) {#dump_state}

Возвращает содержимое контейнера State. При отсутствии параметра `prefix` возвращаются все переменные контейнера.

Переменные типов Array и Map возвращаются в следующем виде:

```
<!-- Array-->
<array_var>s1</array_var>
<array_var>s2</array_var>

<!-- Map -->
<map_var>
     <a>s1</a>
     <b>s2</b>
</map_var>
```

**Входные параметры**: `prefix` (опциональный параметр) - префикс имен переменных, которые необходимо получить из State.

**Пример**:

```
<mist method="dump_state" />
```

#### `echo_cookies` (`echoCookies`) {#echo_cookies}
Получает куки из объекта Request. При отсутствии параметра `prefix`, будут получены все куки.

**Входные параметры**: `prefix` (опциональный параметр) - префикс имен кук, которые необходимо получить из объекта Request.

**Пример**:

```
<mist>
   <method>echo_cookies</method>
   <param type="String">cookie_</param>
</mist>
```

#### `echo_headers` (`echoHeaders`) {#echo_headers}

Получает HTTP-заголовки из объекта Request. При отсутствии параметра `prefix`, будут получены все заголовки.

**Входные параметры**: `prefix` (опциональный параметр) - префикс имен HTTP-заголовков, которые необходимо получить из объекта Request.

**Пример**:

```
<mist>
   <method>echo_headers</method>
   <param type="String">header_</param>
</mist>
```

#### `echo_local_args` (`echoLocalArgs`) {#echo_local_args}

Выводит список переменных из контейнера [LocalArgs](../concepts/block-local-ov.md). При этом имя переменной выводится в виде "prefix<имя переменной в LocalArgs>".

**Входные параметры**: `prefix` - префикс, который добавляется к имени переменной из LocalArgs в XML-выдаче.

**Пример**:

```
<mist>
    <method>echo_local_args</method>
     <param type="String">prefix_</param> 
</mist>
```

#### `echo_protocol` (`echoProtocol`) {#echo_protocol}

Получает переменные окружения протокола FastCGI из объекта Request. При отсутствии параметра `prefix`, будут получены все переменные.

**Входные параметры**: `prefix` (опциональный параметр) - префикс имен переменных окружения, которые необходимо получить.

**Пример**:

```
<mist>
   <method>echo_protocol</method>
   <param type="String">pro_</param>
</mist>
```

#### `echo_query` (`echoQuery`) {#echo_query}

Получает параметры запроса из входной строки `query`.

**Входные параметры**: 
- `prefix` - префикс, который добавляется к имени параметра запроса;
- `query` - строка параметров запроса.

**Пример**:

```
<mist>
   <method>echo_query</method>
   <param type="String">que_</param>
   <param type="StateArg">query</param>
</mist>
```

#### `echo_request` (`echoRequest`) {#echo_request}

Получает содержимое объекта [Request](../concepts/request-ov.md). При отсутствии параметра `prefix`, будут получено всё содержимое Request.

**Входные параметры**: `prefix` (опциональный параметр) - префикс имен параметров из Request, которые необходимо получить.

**Пример**:

```
<mist>
   <method>echo_request</method>
   <param type="String">req_</param>
</mist>
```

#### `erase_state` (`eraseState`) {#erase_state}

Удаляет переменную из контейнера [State](../concepts/state-ov.md).

**Входные параметры**: `varname` - имя удаляемой переменной.

**Пример**:

```
<mist>
   <method>erase_state</method>
   <param type="String">myvariable</param>
</mist> 
```

#### `location` {#location}

Перенаправляет пользователя по адресу `where`.

**Входные параметры**: `where` - адрес, на который перенаправляется запрос.

**Пример**:

```
<mist method="location">
   <param type="StateArg">path</param>
</mist>
```

#### `set_state_by_cookies` (`setStateByCookies`) {#set_state_by_cookie}

Добавляет значения кук пользователя в переменные контейнера State под именами `prefix<имя куки>`.

**Входные параметры**: `prefix` - префикс, который добавляется к имени куки для формирования имени переменной в State, куда записывается значение куки.

**Пример**:

```
<mist>
   <method>set_state_by_cookies</method>
   <param type="String">cookie_</param>
</mist>
```

#### `set_state_by_date` (`setStateByDate`) {#set_state_by_date}

Добавляет в контейнер State две переменные:
- переменную `name`, значением которой является текущая дата в формате (%Y-%m-%d). При этом время учитывается серверное;
- переменную `name_timestamp` со значением - текущей датой в UNIX-формате.

**Входные параметры**: `name` - имя переменной.

**Пример**:

```
<mist>
   <method>set_state_by_date</method>
   <param type="String">date</param>
</mist>
```

#### `set_state_by_headers` (`setStateByHeaders`) {#set_state_by_headers}

Добавляет значения HTTP-заголовков запроса в переменные контейнера State под именами `prefix<имя HTTP-заголовка>`.

**Входные параметры**: `prefix` - префикс, который добавляется к имени HTTP-заголовка для формирования имени переменной в State, куда записывается значение заголовка.

**Пример**:

```
<mist>
   <method>set_state_by_headers</method>
   <param type="String" />
</mist>
```

#### `set_state_by_key` (`setStateByKey`) {#set_state_by_key}

Формирует пары key=value из списков `keys` и `values` в порядке их перечисления. Если в одном из списков количество элементов больше, чем во втором, лишние элементы отбрасываются. В полученном списке производится поиск ключа `key`, и, если его значение не пусто, оно записывается в переменную `name` контейнера State.

**Входные параметры**:
- `name` - имя переменной State, которой присваивается полученное значение;
- `keys` - ключи, перечисленные через запятую;
- `values` - соответствующие ключам значения, перечисленные через запятую;
- `key` - ключ, поиск которого производится в сформированном списке пар key=value.

**Пример**:

```
<mist>
   <method>set_state_by_key</method>
   <param type="String">uid</param>
   <param type="StateArgc">uid_keys</param>
   <param type="StateArg">uid_values</param>
   <param type="UID" as="String"/>
</mist>
```

#### `set_state_by_keys` (`setStateByKeys`) {#set_state_by_keys}

Делает то же самое, что [set_state_by_key](#set_state_by_key), но в качестве последнего параметра принимает список значений `key_list`, поиск ключей из которого производится в сформированном списке key=value, при чем если найдено значение для одного из ключей, поиск значений для остальных ключей не производится.

**Входные параметры**:
- `name` - имя переменной State, которой присваивается полученное значение;
- `keys` - ключи, перечисленные через запятую;
- `values` - соответствующие ключам значения, перечисленные через запятую;
- `key_list` - перечисленные через запятую ключи, поиск которых производится в сформированном списке пар key=value.

**Пример**:

В приведенном ниже примере в множестве {9999=yandex, 213=moscow} выполняется поиск ключей, записанных в переменную `all_region`, и значение первого найденного ключа записывается в переменную `key` контейнера State.

```
<mist>
    <method>set_state_by_keys</method>
    <param type="String">key</param>
    <param type="String">9999,213</param>
    <param type="String">yandex,moscow</param>
    <param type="StateArg" as="String">all_region</param>
</mist>
```

#### `set_state_by_local_args` (`setStateByLocalArgs`) {#set_state_by_local_args}

Добавляет переменные из конейнера [LocalArgs](../concepts/block-local-ov.md) в контейнер [State](../concepts/state-ov.md) под именами "prefix<имя переменной в LocalArgs>".

При добавлении переменных в State
 информация о типе переменных сохраняется.

Информация о новых переменных State отображается в XML-выдаче.

**Входные параметры**:`prefix` - префикс, который добавляется к имени переменной из LocalArgs при записи её в State.

**Пример**:

```
<mist>
     <method>set_state_by_Local_args</method>
     <param type="String">prefix_</param> 
</mist>
```

#### `set_state_by_query` (`setStateByQuery`) {#set_state_by_query}

Добавляет значения параметров запроса из строки `query` в переменные контейнера State под именами `prefix<имя параметра запроса>`.

**Входные параметры**:
- `prefix` - префикс, который добавляется к имени параметра запроса для формирования имени переменной в State, куда записывается значение параметра;
- `query` - строка параметров запроса.

**Пример**:

```
<mist>
    <method>set_state_by_query</method>
    <param type="String">query_</param>
    <param type="String">var1=a&var2=b</param>
</mist>
```

#### `set_state_by_protocol` (`setStateByProtocol`) {#set_state_by_protocol}

Добавляет значения переменных окружения протокола FastCGI в переменные контейнера State под именами `prefix<имя переменной окружения>`.

**Входные параметры**: `prefix` - префикс, который добавляется к имени переменной окружения для формирования имени переменной в State, куда записывается значение переменной окружения.

**Пример**:

```
<mist>
   <method>set_state_by_protocol</method>
   <param type="String"/>
</mist>
```

В результате выполнения блока будет получен следующий XML-фрагмент:

```
<state prefix="" type="Protocol">

<param name="path">/asessor/index.xml</param>
<path>/asessor/index.xml</path>

<param name="uri">/asessor/index.xml</param>
<uri>/asessor/index.xml</uri>

<param name="originaluri">/asessor/index.xml</param>
<originaluri>/asessor/index.xml</originaluri>

<param name="originalurl">
https://devel.fireball.yandex.ru:8091/asessor/index.xml
</param>
<originalurl>
https://devel.fireball.yandex.ru:8091/asessor/index.xml
</originalurl>

<param name="host">devel.fireball.yandex.ru:8091</param>
<host>devel.fireball.yandex.ru:8091</host>

<param name="originalhost">
devel.fireball.yandex.ru:8091
</param>
<originalhost>
devel.fireball.yandex.ru:8091
</originalhost>

<param name="realpath">
/opt/lighttpd-xscript5/devel/asessor/index.xml
</param>
<realpath>
/opt/lighttpd-xscript5/devel/asessor/index.xml
</realpath>

<param name="secure">yes</param>
<secure>yes</secure>

<param name="bot">no</param>
<bot>no</bot>

<param name="method">GET</param>
<method>GET</method>

<param name="remote_ip">95.108.174.209</param>
<remote_ip>95.108.174.209</remote_ip>

</state>
```

#### `set_state_by_request` (`setStateByRequest`) {#set_state_by_request}

Добавляет параметры запроса пользователя, извлеченные из объекта [Request](../concepts/request-ov.md), в переменные контейнера State под именами `prefix<имя параметра>`.

Если в запросе присутствует несколько параметров с одинаковым именем, в State будет записан только последний из них.

**Входные параметры**: `prefix` - префикс, который добавляется к имени параметра для формирования имени переменной в State, куда записывается значение параметра.

**Пример**:

```
<mist>
    <method>set_state_by_request</method>
    <param type="String"/>
</mist>
```

#### `set_state_by_request_urlencoded` (`setStateByRequestUrlencoded`) {#set_state_by_request_urlencoded}

Выполняет urlencode параметров запроса, переводит их в кодировку `encoding` и сохраняет в переменные контейнера State под именами `prefix<имя параметра>`.

Если в запросе присутствует несколько параметров с одинаковым именем, в State будет записан только последний из них.

**Входные параметры**:
- `prefix` - префикс, который добавляется к именам переменных, куда записываются urlencoded-параметры;
- `encoding` (опциональный параметр) - кодировка, в которую переводятся параметры запроса. Если данный параметр не указан, используется кодировка UTF8.

**Пример**:

```
<mist>
    <method>set_state_by_request_urlencoded</method>
    <param type="String"/>
    <param type="String">cp1251</param>
</mist>
```

#### `set_state_concat_string` (`setStateConcatString`) {#set_state_concat_string}

Добавляет в контейнер State переменную `name`, в которую помещается строка, полученная путем конкатенации параметров `str-1`...`str-n`.

**Входные параметры**: `<name> <str-1> <str-2> [<str-3>...]`
- `name` - имя переменной, в которую записывается строка, получившаяся в результате конкатенации;
- `str-1...str-n` - строки, которые необходимо конкатенировать (не менее двух).

**Пример**:

```
<mist method="set_state_concat_string">
   <param type="String">lingvo-x3-url</param>
   <param type="String">http://</param>
   <param type="StateArg" as="Long">lingvo-x3-backend1</param>
   <param type="String">/</param>
</mist>
```

#### `set_state_defined` (`setStateDefined`) {#set_state_defined}

Добавляет в переменную `name` контейнера State значение первое не пустое значение переменной из списка `names`.

**Входные параметры**:
- `name` - имя переменной State, в которую записывается полученное значение;
- `names` - имена переменных State, перечисленные через запятую.

**Пример**:

```
<mist method="set_state_defined">
   <param type="String">name</param>
   <param type="String">name,def_name</param>
</mist>
```

#### `set_state_domain` (`setStateDomain`) {#set_state_domain}

Выделяет домен n-ного уровня из названия хоста или URL-а.

**Входные параметры**:
- `name` - имя переменной в контейнере State, в которую будет записан выделенный домен;
- `url` - URL, из которого происходит выделение домена. Не должен содержать точек в начале или конце строки или нескольких идущих подряд точек. Не может содержать IP-адрес. Если имя домена не удовлетворяет этим требованиям, будет возвращено сообщение об ошибке;
- `level` - необходимый уровень домена (опциональный параметр). Если запрашиваемый уровень домена превышает максимальный уровень домена в URL, будет возвращен домен максимально возможного уровня, а в лог записано предупреждение (Warning). Если параметр отсутствует, в State будет записано полное доменное имя.

**Пример**:

```
<mist>
   <method>set_state_domain</method>
   <param type="String">tmp</param>
   <param type="String">http://www.yandex.ru/</param>
   <param type="Long">2</param>
</mist>
```

#### `set_state_double` (`setStateDouble`) {#set_state_double}

Добавляет в контейнер State переменную `name` со значением `value` типа Double (число с плавающей точкой).

**Входные параметры**:
- `name` - имя переменной;
- `value` - значение переменной (типа Double).

**Пример**:

```
<mist>
   <method>set_state_double</method>
   <param type="String">name</param>
   <param type="Double">2,5</param>
</mist>
```

#### `set_state_long` (`setStateLong`) {#set_state_long}

Добавляет в контейнер [State](../concepts/state-ov.md) переменную `name` со значением `value` типа Long (целое число).

**Входные параметры**:
- `name` - имя переменной;
- `value` - значение переменной (типа Long).

**Пример**:

```
<mist>
   <method>set_state_long</method>
   <param type="String">name</param>
   <param type="Long">2</param>
</mist>
```

#### `set_state_longlong` (`setStateLongLong`) {#set_state_longlong}

Добавляет в контейнер State переменную `name` со значением `value` типа LongLong (целое число, 64 бита). Тип LongLong используется для записи чисел, превышающих 4 млрд, например, для хранения UID.

**Входные параметры**:
- `name` - имя переменной;
- `value` - значение переменной (типа LongLong).

**Пример**:

```
<mist>
   <method>set_state_long</method>
   <param type="String">name</param>
   <param type="LongLong">22</param>
</mist>
```

#### `set_state_join_string` (`setStateJoinString`) {#set_state_join_string}

Добавляет в контейнер State переменную `name`, в которую помещаются строка, получившаяся после слияния значений переменных, имена которых начинаются с `prefix`, соединенных через `delim`.

**Входные параметры**:
- `name` - имя переменной State, в которую помещается полученная строка;
- `prefix` - часть имен переменных в State, которые должны быть слиты.
- `delim` - разделитель, который помещается между значениями переменных с именами `prefix`.

**Пример**:

```
<mist>
   <method>set_state_join_string</method>
   <param type="String">scraps_ids</param>
   <param type="String">test_vals</param>
   <param type="String"></param>
</mist>
```

#### `set_state_random` (`setStateRandom`) {#set_state_random}

Добавляет в контейнер State переменную `name`, значением которой является случайное целое число из диапазона [`low`,`high`).

**Входные параметры**:
- `name` - имя переменной;
- `low` - нижняя граница диапазона. Должна находиться в интервале [-2147483647, 2147483647);
- `high` - верхняя граница диапазона. Должна находиться в интервале (-2147483647, 2147483647].

**Пример**:

```
<mist method="set_state_random">
   <param type="String">lingvo-x3-random</param>
   <param type="String">1</param>
   <param type="StateArg" as="Long">lingvo-x3-backends-count</param>
</mist>
```

#### `set_state_split_string` (`setStateSplitString`) {#set_state_split_string}

Добавляет в контейнер State переменные с именами `prefix0`, `prefix1`, `prefix2` и т.д. и значениями, образовавшиеся после разделения строки `value` разделителем `delim`.

**Входные параметры**:
- `prefix` - префикс, к которому добавляются порядковые номера для формирования имен переменных, в которые будут сохранены полученные в результате разделения строки;
- `value` - строка для разделения;
- `delim` - разделитель (строкового типа).

**Пример**:

В приведенном ниже примере строка [originaluri](protocol-arg.md#originaluri) разбивается на две части - до и после символа "?", и полученные подстроки сохраняются в переменные `splitted_uri0` и `splitted_uri1`.

```
<mist>
    <method>set_state_split_string</method>
    <param type="String">splitted_uri</param>
    <param type="ProtocolArg" as="String">originaluri</param>
    <param type="String">?</param>
</mist>
```

#### `set_state_string` (`setStateString`) {#set_state_string}

Добавляет в контейнер State переменную `name` со значением `value` типа String (строка).

**Входные параметры**:
- `name` - имя переменной;
- `value` - значение переменной (типа String).

**Пример**:

```
<mist method="set_state_string">
   <param type="String">lingvo-x3-dict</param>
   <param type="QueryArg" as="String">dict</param>
</mist>
```

#### `set_state_urldecode` (`setStateUrldecode`) {#set_state_urldecode}

Добавляет в контейнер State переменную name, в которую помещается urldecoded строка value в кодировке encoding.

**Входные параметры**:
- `name` - имя переменной, в которую помещается urldecoded строка;
- `value` - строка, к которой применяется urldecode и перевод в другую кодировку;
- `encoding` (опциональный параметр) - кодировка, в которую переводится строка `value`. Если этот параметр не указан, используется кодировка UTF8. Параметр регистронезависим.

**Пример**:

```
<method>set_state_urldecode</method>
   <param type="String">params</param>
   <param type="String">/yandsearch?rpt=slovari&amp;charset=UTF-8&amp;</param>
</mist>
```

#### `set_state_urlencode` (`setStateUrlencode`) {#set_state_urlencode}

Добавляет в контейнер State переменную `name`, в которую помещается urlencoded строка `value` в кодировке `encoding`.

**Входные параметры**:
- `name` - имя переменной, в которую помещается urlencoded строка;
- `value` - строка, к которой применяется urlencode и перевод в другую кодировку;
- `encoding` (опциональный параметр) - кодировка, в которую переводится строка `value`. Если этот параметр не указан, используется кодировка UTF8. Параметр регистронезависим.

**Пример**:

```
<mist method="set_state_urlencode">
   <param type="String">lingvo-x3-text</param>
   <param type="QueryArg" as="String">text</param>
   <param type="String">utf-8</param>
</mist>
```

#### `set_state_xmlescape` (`setStateXmlescape`) {#set_state_xmlescape}

Применяет к строке xml-escaping и записывает результат в State. Xml-escaping подразумевает следующую замену символов в строке:

```
> на &gt;
< на &lt;
& на &amp;
' на &#39;
" на &quot;
```

**Входные параметры**:
- `string` - строка, к которой необходимо применить xml-escaping;
- `name` - имя переменной в State, куда должен быть записан результат.

**Пример**:

```
<mist method="set_state_xmlescape">
   <param type="StateArg">text</param>
   <param type="String">name</param>
</mist>
```

#### `set_status` (`setStatus`) {#set_status}

Устанавливает HTTP-статус ответа на запрос.

**Входные параметры**: `status` - числовой код статуса.

**Пример**:

В приведенном ниже примере с помощью [guard](../reference/guard.md)-условия проверяется, возникла ли в ходе обработки запроса ошибка, и если она возникла, на страницу накладывается `/usr/local/www/common/404/404.xsl` и устанавливается HTTP-статус 404.

```
<mist method="attachStylesheet" guard="error" xpointer="/..">
    <param type="String">/usr/local/www/common/404/404.xsl</param>
</mist>

<mist method="setStatus" guard="error" xpointer="/..">
    <param type="String">404</param>
</mist>
```

### Узнайте больше {#learn-more}
* [Mist-блок](../concepts/block-mist-ov.md)
* [mist](../reference/mist.md)