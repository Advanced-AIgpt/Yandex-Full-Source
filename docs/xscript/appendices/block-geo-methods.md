# Методы Geo-блока

**Список методов Geo-блока**:
- [is_in](block-geo-methods.md#is_in);
- [is_yandex](block-geo-methods.md#is_yandex);
- [get_geo_list](block-geo-methods.md#get_geo_list);
- [get_geo_location](block-geo-methods.md#get_geo_location);
- [get_geo_tree](block-geo-methods.md#get_geo_tree);
- [get_geo_tree_with_depth](block-geo-methods.md#get_geo_tree_with_depth);
- [getLocalProxyBaseUrl](block-geo-methods.md#get_local_proxy_baseurl);
- [get_native_region](block-geo-methods.md#get_native_region);
- [get_region](block-geo-methods.md#get-region);
- [search_regions](#search_regions);
- [set_state_by_tzname;](block-geo-methods.md#set_state_by_tzname)
- [set_state_parents;](block-geo-methods.md#set_state_parents)
- [set_state_region](block-geo-methods.md#set_state_region);
- [set_state_region_name](block-geo-methods.md#set_state_region_name).

Методы [get_geo_list](block-geo-methods.md#get_geo_list), [get_geo_tree](block-geo-methods.md#get_geo_tree), [get_geo_tree_with_depth](block-geo-methods.md#get_geo_tree_with_depth) и [get-region](block-geo-methods.md#get-region) позволяют получить XML-дерево регионов. Список полей приведён ниже.

#### Описание региона {#region-fields}

При описании региона в выдаче могут быть использованы следующие поля:

- id - идентификатор региона;
- enname — название региона на английском языке;
- runame — русскоязычное название региона, именительный падеж;
    - runame_genitive — русскоязычное название региона, родительный падеж;
    - runame_dative — русскоязычное название региона, дательный падеж;
    - runame_locative — русскоязычное название региона, предложный падеж;
    - runame_preposition - русский предлог;
    
- ukname — название региона на украинском языке, именительный падеж;
    - ukname_genitive — название региона на украинском языке, родительный падеж;
    - ukname_dative — название региона на украинском языке, дательный падеж;
    - ukname_locative — название региона на украинском языке, предложный падеж;
    - ukname_preposition — украинский предлог;
    
- bename — название региона на белорусском языке, именительный падеж;
    - bename_genitive — название региона на белорусском языке, родительный падеж;
    - bename_dative — название региона на белорусском языке, дательный падеж;
    - bename_locative — название региона на белорусском языке, предложный падеж;
    - bename_preposition — белорусский предлог;
    
- kkname — название региона на казахском языке, именительный падеж;
    - kkname_genitive — название региона на казахском языке, родительный падеж;
    - kkname_dative — название региона на казахском языке, дательный падеж;
    - kkname_locative — название региона на казахском языке, местный падеж;
    
- ttname — название региона на татарском языке, именительный падеж;
    - ttname_genitive — название региона на татарском языке, родительный падеж;
    - ttname_directional — название региона на татарском языке, направительный падеж;
    - ttname_locative — название региона на татарском языке, местный падеж;
    
- trname — название региона на турецком языке, именительный падеж;
    - trname_genitive — название региона на турецком языке, родительный падеж;
    - trname_prepositional — название региона на турецком языке, предложный падеж;
    - trname_directional — название региона на турецком языке, направительный падеж;
    - trname_locative — название региона на турецком языке, местный падеж;
    
- syn - синонимы для названия региона, перечисленные через запятую;
- short_ename - краткое название региона на английском языке;
- pid/parent - идентификатор родительского региона;
- main - флаг, указывающий, что регион является главным на данном уровне дерева;
- chief_region - id главного региона для текущего региона;
- pos, position - позиция в Геобазе, показывающая популярность региона;
- type - тип региона. Данное поле может иметь следующие значения:
    - -1 - скрытый регион;
    - 0 - прочее;
    - 1 - континент;
    - 2 - регион;
    - 3 - страна;
    - 4 - федеральный округ;
    - 5 - субъект федерации;
    - 6 - город;
    - 7 - село;
    - 8 - район города;
    - 9 - станция метро;
    - 10 - район субъекта федерации;
    - 11 - аэропорт.
    
- timezone - флаг, указывающий, определен ли часовой пояс для данного региона (0 - не определен, 1 - определен);
- phone_code - телефонный код региона;
- zip - почтовый код региона;
- lat - широта, на которой находится регион;
- lon - долгота, на которой находится регион;
- spn_lat - спан для широты;
- spn_lon - спан для долготы;
- zoom - зум;
- tz_name - имя часового пояса региона;
- tz_abbr - принятая аббревиатура часового пояса (например, "MSK" - Москва);
- tz_offset - текущее смещение относительно UTC в секундах;
- tz_dst - флаг, указывающий летнее или зимнее время в данный момент в регионе. Возможные значения: "std" - зимнее время,"dst" - летнее время;
- bs -флаг, указывающий, есть ли для региона Баннерокрутилка (0 - нет, 1 - есть);
- yaca - флаг, указывающий, есть ли для региона Каталог (0 - нет, 1 - есть);
- weather - флаг, указывающий, есть ли для региона Погода (0 - нет, 1 - есть);
- afisha - флаг, указывающий, есть ли для региона Афиша (0 - нет, 1 - есть);
- maps - флаг, указывающий, есть ли для региона Карты (0 - нет, 1 - есть);
- tv - флаг, указывающий, есть ли для региона Телепрограмма (0 - нет, 1 - есть).

#### `is_in` (`isIn`) {#is_in}

Проверяет регион на принадлежность к указанному региону. Результат возвращается в виде кода (<q>0</q> — внутри, <q>1</q> — снаружи, <q>2</q> — не удалось определить принадлежность). Каждому коду соответствует символьный идентификтор: <q>0</q> — <q>inside</q>, <q>1</q> — <q>outside</q>, <q>2</q> — <q>unknown</q>.

**Входные параметры**:
- числовой идентификатор региона,на принадлежность к которому производится проверка;
- (опциональный параметр) числовой идентификатор региона, для которого определяется принадлежность. Если параметр не используется, текущий регион пользователя определяется автоматически.

**Пример**:

```
<geo-block>
    <method>is_in</method>
    <param type="Long">213</param>
</geo-block>

<geo-block>
    <method>isIn</method>
    <param type="Long">2</param>
    <param type="Long">213</param>
</geo-block>

<geo-block>
    <method>isIn</method>
    <param type="Long">2</param>
    <param type="Long">101511</param>
</geo-block>

<geo-block>
    <method>isIn</method>
    <param type="Long">10511</param>
    <param type="Long">2</param>
</geo-block>

<geo-block>
    <method>isIn</method>
    <param type="Long">213</param>
    <param type="Long">213</param>
</geo-block>

<geo-block>
    <method>isIn</method>
    <param type="Long">213</param>
    <param type="Long">20111102</param>
</geo-block>
```

В результате выполнения блока будет возвращен следующий XML-фрагмент:

```
<result code="0">inside</result>

<result code="1">outside</result>

<result code="0">inside</result>

<result code="1">outside</result>

<result code="0">inside</result>

<result code="2">unknown</result>
```

Код, содержащейся в первом результате, будет равен нулю, если страница вызывается пользователем, положение которого определилось как Москва.

#### `is_yandex` (`isYandex`) {#is_yandex}

Определяет, находится ли пользователь в одной из внутриофисных сетей Яндекса.

Метод не имеет входных параметров. Определение производится на основании данных, полученных из HTTP-запроса.

Результат возвращается в виде символьного кода: <q>yes</q> или <q>no</q>.

**Пример**:

```
<geo-block>
    <method>isYandex</method>
</geo-block>
```

Для пользователя, находящегося в одной из внутриофисных сетей Яндекса, будет возвращен следующий фрагмент:

```
<result is-yandex="yes"/>
```

#### `get_geo_list` (`getGeoList`) {#get_geo_list}

Возвращает список регионов с указанными id.

**Входные параметры**:
- имена полей (через запятую), которые должны присутствовать в списке;
- id регионов (через запятую).

**Пример**:

В приведенном примере выполняется получение информации (name, position, timezone) о регионах с id 1, 2 и 3.

```
<geo-block>
    <method>getGeoList</method>
    <param type="String">name, position, timezone</param>
    <param type="String">1,2,3</param>
</geo-block>
```

В результате выполнения блока будет возвращен следующий XML-фрагмент:

```
<regions>
    <region id="1" name="Москва и Московская область" position="213" timezone="312"/>
    <region id="3" name="Центр" position="382" timezone="312"/>
    <region id="2" name="Санкт-Петербург" position="10885"/>
</regions>
```

#### `get_geo_location` (`getGeoLocation`) {#get_geo_location}

Позволяет определить местоположение пользователя (регион и географические координаты). При определении учитываются следующие данные:

- IP-адрес пользователя;
- Информация, содержащаяся в HTTP-заголовках `X-Forwarded-For` и `X-Real-IP`.
- Регион пользователя, содержащийся в куке yandex_gid.
- Информация о местоположении пользователя, содержащаяся в Y-куках ys и yp. Эта информация передается в куки Баром, а также сервисами Яндекса на основе данных, введенных пользователем (например, [http://tune.yandex.ru/region/](http://tune.yandex.ru/region/)). При этом учитываются данные о местоположении пользователя, определенные с помощью анализа сигналов от WiFi и GSM сетей. Если устройство снабжено соответствующим оборудованием, такая информация доступна значительному числу современных браузеров.

**Входные параметры**:
- IP-адрес, для которого определяется местоположение;
- (опциональный параметр) содержимое куки yp, представленное в виде строки;
- (опциональный параметр) содержимое куки ys, представленное в виде строки.

**Возвращаемые данные**

Метод возвращает следующие данные в виде атрибутов элемента `geolocation`:

- `region` — регион пользователя, определённый по совокупности доступных данных.
- `region-by-ip` — регион пользователя, определённый по указанному IP-адресу.
- `suspected-region` — возможный регион пользователя. Определяется по совокупности данных, но вероятность нахождения пользователя в этом регионе ниже, чем в регионе, указанном в атрибуте `region`.
- `should-update-cookie` — информация о том, рекомендуется ли обновить пользователю куку yandex_gid. Может принимать значения <q>0</q> (не найдено предпосылок для обновления куки) и <q>1</q> (куку рекомендуется обновить). Обновить куку может быть рекомендовано, например, в том случае, если есть весомые основания предполагать, что пользователь находится не в том регионе, который задан в куке yandex_gid.
- `latitude` и `longitude` — географические координаты пользователя (широта и долгота), насколько их удалось определить. Если удалось определить только регион, возвращается координаты центра региона.

**Примеры**:

```
<geo-block>
    <method>getGeoLocation</method>
    <param type="String">77.51.255.18</param>
</geo-block>
```

Если пользователь находится в московском офисе Яндекса, результат будет выглядеть (примерно) следующим образом:

```
<geolocation region="9999" region-by-ip="213" suspected-region="-1" should-update-cookie="0" latitude="55.7339138" longitude="37.5880432"/>
```

Чтобы при определении местоположения учесть данные, содержащиеся в куках yp и ys, можно использовать код, сходный со следующим:

```
<mist>
    <method>set_state_by_cookies</method>
    <param type="String">cookie_</param>
</mist>

<geo-block>
    <method>getGeoLocation</method>
    <param type="String">87.11.41.13</param>
    <param type="StateArg" as="String">cookie_yp</param>
    <param type="StateArg" as="String">cookie_ys</param>
</geo-block>
```

#### `get_geo_tree` (`getGeoTree`) {#get_geo_tree}

Возвращает XML-дерево регионов, начиная от заданного корневого региона.

**Входные параметры**:
- имена полей (через запятую), которые должны присутствовать в XML-дереве;
- (опциональный параметр) id региона-корневого узла дерева. Если данный параметр не указан, используется значение "10000" (Земля).

**Пример**:

В приведенном примере выполняется запрос дерева всех подрегионов Москвы с указанием их имен.
```
<geo-block>
    <method>getGeoTree</method>
    <param type="String">name</param>
    <param type="Long">213</param>
</geo-block>
```

В результате работы метода будет возвращено дерево следующего вида:
```
<geobase>
    <region id="213" name="Москва">
        <region id="9999" name="Яндекс"/>
        <region id="20363" name="СЗАО">
             <region id="20570" name="Волоколамская"/>
             ...
             <region id="20364" name="Планерная"/>
        </region>
        ...
        <region id="216" name="Зеленоград">
            <region id="21744" name="Речной вокзал"/>
        </region>
    </region>
</geobase>
```

#### `get_geo_tree_with_depth` (`getGeoTreeWithDepth`) {#get_geo_tree_with_depth}
 
Возвращает XML-дерево регионов заданной глубины, начиная от указанного корневого элемента.

**Входные параметры**:
- имена полей (через запятую), которые должны присутствовать в XML-дереве;
- глубина дерева;
- (опциональный параметр) id региона-корневого узла дерева. Если данный параметр не указан, используется значение "10000" (Земля).

**Пример**:

В приведенном примере выполняется запрос всех государств Европы с указанием их имен и позиций в Геобазе.
```
<geo-block> 
    <method>getGeoTreeWithDepth</method>
    <param type="String">name,position</param>
    <param type="Long">1</param>
    <param type="Long">111</param>  
</geo-block>
```

В результате работы метода будет возвращено следующее дерево:
```
<geobase>
    <region id="213" name="Европа" position="183">
        <region id="127" name="Швеция" position="21610"/>
        <region id="126" name="Швейцария" position="21359"/>
        <region id="125" name="Чехия" position="21203"/>
        ...
    </region>
</geobase>
```

#### `getLocalProxyBaseUrl` {#get_local_proxy_baseurl}

Позволяет определить URL регионального сервера, наиболее подходящего для выдачи статики заданному IP.

Если IP-адрес пользователя, принадлежит провайдеру, поддерживающему прямой peer с региональным сервером Яндекса, то отдавать статику этому IP разумнее с регионального сервера, а не с основного балансера.

Техника использования метода такова. Региональные серверы логически объединяются в группы по произвольному признаку (например, хранилища изображений карт). Каждой такой группе присваивается символьный идентификатор (например, maps). Подготавливается конфигурационный XML-файл, в котором устанавливается соответствие между регионами и серверами выделенных групп, выдающих статику в эти регионы. URL серверов выдачи помещаются в элементы вида [идентификатор группы]-static, например:

```
<regions-static>
  <region name="piter" count="3" id="2">
...
    <maps-static count="2">
      <url>http://sat.maps.yandex.net.cstatic-piter02.corba.yandex.net</url>
      <url>http://sat.maps.yandex.net.cstatic-piter01.corba.yandex.net</url>
    </maps-static>
    <yaru-static count="2">
...
    </yaru-static>
  </region>

```

При обращении к методу указывается IP-адрес и идентификатор группы. По IP-адресу определяется регион. Если для региона существуют (прописаны в конфигурационном файле) серверы для выдачи статики, возвращается URL одного из этих серверов и сопутствующая информация. В качестве сопутствующей информации выводится базовая часть URL (без префикса протокола и завершающего слеша) и короткое название региона на латинице (так, как это прописано в конфигурационном файле).

Если региону соответствует более одного сервера выдачи данной группы, то случайным образом выбирается один. Таким образом осуществляется распределения нагрузки между этими серверами.

Для определения региона, соответствующего заданному IP-адресу, используется файл, сгенерированный из геобазы и базы геотаргетинга. Путь к этому файлу указывается в элементе `geodata`[конфигурационного файла XScript](config-params.md#xscript-geo). Там же указывается и путь к конфигурационному файлу, устанавливающему соответствие между регионами и серверами выдачи статики для каждой из групп серверов.

**Входные параметры**: 
- IP-адрес, для которого необходимо определить наиболее подходящий региональный сервер выдачи статики;
- Идентификатор группы серверов выдачи статики.

**Пример**:

```
<geo-block>
       <method>getLocalProxyBaseUrl</method>
       <param type="String">123.4.13.4</param>
       <param type="String">maps</param>
</geo-block>
```

В результате выполнения блока будет возвращен следующий XML-фрагмент:

```
<region name="piter" id="2"
basehost="sat.maps.yandex.net.cstatic-piter01.corba.yandex.net"
baseurl="http://sat.maps.yandex.net.cstatic-piter01.corba.yandex.net"/>

```

#### `get_native_region (getNativeRegion)` {#get_native_region}

Позволяет определить id региона пользователя по его IP-адресу и HTTP-заголовкам _X-Forwarded-For_ и _X-Real-IP_. При этом значение куки _yandex_gid_ игнорируется.

Возвращает XML, состоящий из одного элемента `<native-region>`, который содержит id региона:

```
<native-region>213</native-region>
```

Если регион определить не удалось, возвращается пустой элемент `<native-region>`:

```
<native-region/>
```

**Входные параметры**: отсутствуют.

**Пример**:

```
<geo-block method="getNativeRegion"/>
```

#### `get_region` (`getRegion`) {#get-region}

Позволяет получить информацию о регионе по его id.

**Входные параметры**: 
- имена интересующих полей с информацией о регионе, перечисленные через запятую;
- id региона.

**Пример**:

```
<geo-block>
    <method>getRegion</method>
    <param type="String">name, position, timezone</param>
    <param type="String">213</param>
</geo-block>
```

В результате выполнения блока будет возвращен следующий XML-фрагмент:

```
<region id="213" name="Москва" position="20571" timezone="312"/>
```

#### `search_regions (searchRegions)` {#search_regions}

Выдает перечень регионов, удовлетворяющих заданным условиям. По умолчанию метод не работает. Чтобы его включить, необходимо установить параметр `search-enabled=yes`[конфигурации XScript](config-params.md#xscript-geo).

**Входные параметры:**

- `part` - подстрока, которая может содержаться в названии искомых регионов;
- `count` - количество регионов в результате;
- `fields` - названия полей через запятую, которые нужно добавить в результат. Перечень полей с описанием находится в [разделе "Описание региона"](#region-fields);
- `types` - типы искомых регионов, через запятую. Список кодов приведён в [разделе "Описание региона"](#region-fields) в описании поля `type`;
- `lang` - двухбуквенный код языка. На текущий момент поддерживаются `ru`, `uk`, `en`, `by`, `kz`, `tt`, `tr`. Используется как начальная настройка поиска по подстроке (`part`);
- `parents` - коды родительских регионов, иерархическая принадлежность к которым проверяется для всех найденных регионов.

**Пример:**

```
<?xml version="1.0" encoding="UTF-8"?>
<page xslt-dont-apply="yes">
    <geo-block>
        <method>search_regions</method>

        <param type="QueryArg" default="Симф">part</param>
        <param type="QueryArg" default="15">count</param>
        <param type="QueryArg" default="6,7">types</param>
        <param type="QueryArg" default="ru">lang</param>
        <param type="QueryArg" default="10000">parents</param>
        <param type="QueryArg" default="name,population">fields</param>
    </geo-block>
</page>
```

Результат:

```
<page xslt-dont-apply="yes">
    <search_results>
        <region id="146" name="Симферополь" population="335500" lang="ru">
                <parent id="977" name="Крым" population="0" lang="ru"/>
        </region>
    </search_results>
</page>

```

#### `set_state_by_tzname` (`setStateByTzName`) {#set_state_by_tzname}

На основании идентификатора временной зоны определяет следующие её характеристики: текущая аббревиатура, текущее смещение относительно UTC и текущее используемое время ('dst' — в случае летнего времени, 'std' — в случае зимнего и 'never' — если в данной зоне нет перехода). Полученные характеристики записываются в переменные State `<префикс>_abbr` (аббревиатура), `<префикс>_offset` (смещение), `<префикс>_dst` (текущее используемое время).

**Входные параметры**:
- префикс, который используется для формирования имен переменных в State;
- идентификатор временной зоны в UNIX-формате (например "Europe/Moscow", "Europe/Dublin" и т.д.).

**Пример**:

В приведенном ниже примере выполняется получение информации о временной зоне Europe/Moscow.

```
<geo-block>
    <method>setStateByTzName</method>
    <param type="String">moscow</param>
    <param type="String">Europe/Moscow</param>
</geo-block>
<mist method="dumpState"/>
```

В результате выполнения блока будет возвращен следующий XML-фрагмент:

```
<timezone name="Europe/Moscow" prefix="moscow">
    <state type="String" name="moscow_abbr">MSK</state>
    <state type="String" name="moscow_dst">std</state>
    <state type="Long" name="moscow_offset">10800</state>
</timezone>
<state_dump>
    <param name="moscow_abbr" type="String">MSK</param>
    <param name="moscow_dst" type="String">std</param>
    <param name="moscow_offset" type="Long">10800</param>
</state_dump>
```

#### `set_state_parents` (`setStateParents`){#set_state_parents}

Получает список регионов, в которые входит регион с указанным id, и записывает его в переменную State.

**Входные параметры**: 
- имя переменной контейнера State, в которую записывается полученный список регионов;
- (опциональный параметр) id региона, для которого выполняется получение вышестоящих регионов (например, для id города будут получены id области, id страны, и т.д.). Если данный параметр отсутствует, список регионов формируется на основании id из куки **yandex_gid**, а если она не установлена - по id региона IP-адреса, с которого пришел запрос (remoteIP).

**Пример**:
```
<geo-block>
  <method>set_state_parents</method>
  <param type="String">all_region_state</param>
  <param type="StateArg" default="213">region</param> 
</geo-block>
```

То же самое с использованием HTTP-заголовка X-Region:
```
<geo-block>
  <method>set_state_parents</method>
  <param type="String">all_region_state</param>
  <param type="HTTPHeader" default="213">X-Region</param>
</geo-block>
```

Результатом выполнения блока в обоих случаях будет следующий XML-фрагмент:
```
<state type="Geo" name="all_region_state">
  57,11193,52,225,10001,10000
</state>
```

#### `set_state_region` (`setStateRegion`) {#set_state_region}

Получает id региона по IP-адресу и записывает полученное значение в указанную переменную контейнера [State](../concepts/state-ov.md).

**Входные параметры**: 
- имя переменной контейнера State, в которую записывается полученное значение;
- (опциональный параметр) IP-адрес, для которого выполняется получаение id региона. Если данный параметр не указан, выполняется получение id из куки _yandex_gid_, а если она не установлена - по IP-адресу, с которого пришел запрос (remoteIP).

**Пример**:
```
<geo-block>
  <method>set_state_region</method>
  <param type="String">region</param>
</geo-block>
```

Результатом выполнения блока будет следующий XML-фрагмент:
```
<state type="Region" name="region">57</state>
```

#### `set_state_region_name` (`setStateRegionName`) {#set_state_region_name}

Получает имя региона на основании id региона и записывает его в переменную контейнера State.

**Входные параметры**: 
- имя переменной контейнера State, в которую записывается полученное значение;
- (опциональный параметр) id региона, для которого выполняется получение имени. Если этот параметр отсутствует, имя региона определяется на основании id из куки **yandex_gid**, а если она не установлена - по id региона IP-адреса, с которого пришел запрос (remoteIP).

**Пример**:
```
<page>
        <geo-block>
                <method>set_state_region</method>
                <param type="String">region</param>
        </geo-block>
        <geo-block>
                <method>set_state_region_name</method>
                <param type="String">region_name</param>
                <param type="StateArg" default="213">region</param>
        </geo-block>
</page>
```

### Узнайте больше {#learn-more}
* [Geo-блок](../concepts/block-geo-ov.md)
* [geo-block](../reference/geo.md)
* [Подготовка данных при помощи пакета geobase-builder](https://wiki.yandex-team.ru/LeonidMovsesjan/XGeo/GeoBuilder)
* [Настройка xscript-geo](../appendices/config-params.html#xscript-geo)
* [Использование xscript-geo](https://wiki.yandex-team.ru/LeonidMovsesjan/XGeo/XGeoUsage)