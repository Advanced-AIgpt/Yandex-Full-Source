# Модули XScript

## xscript-corba.so {#xscript-corba}

Содержит [CORBA-блок](block-corba-ov.md).

**Входит в пакет**

[xscript-corba](packages.md#xscript-corba)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-corba.so](../appendices/config-params.md#xscript-corba)


## xscript-development.so {#xscript-development}

Модуль, включающий режим "development" для более строгой проверки ошибок парсинга и выполнения страниц. Если он подключен, все ошибки выводятся в тело HTTP-ответа, что позволяет видеть их в окне браузера.

В режиме "development" некоторые потенциально опасные ситуации (на странице произошла ошибка, но страница продолжает работать) приводят к ответу с кодом 500.

**Входит в пакет**

[xscript-development](packages.md#development)


## xscript-diskcache.so {#xscript-diskcache}

Кэш для хранения на диске [результатов выполнения блоков](block-results-caching.md) и [выходных страниц](caching-ov.md).

Сохраняется при перезапуске XScript.

Локальный - на каждом front-end-е своя хранится своя копия кэша.

При одновременном использовании с [memcache](modules.md#xscript-memcache) приоритет обращения к кэшам для получения их элементов определяется порядком, в котором кэши подключены в конфигурационном файле.

На x64-серверах, где нет ограничений по памяти, имеет смысл использовать вместо дискового кэша [memcache](modules.md#xscript-memcache), так как при большой нагрузке
 частые обращения к диску будут замедлять работу системы.

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-diskcache.so](../appendices/config-params.md#xscript-diskcache)

## xscript-file.so {#xscript-file}

Содержит [File-блок](block-file-ov.md).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-geo.so {#xscript-geo}

Содержит [Geo-блок](block-geo-ov.md) и набор [XSL-функций](../appendices/xslt-functions.md).

**Входит в пакет**

[xscript-geo](packages.md#xscript-geo)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-geo.so](../appendices/config-params.md#xscript-geo)

## xscript-http.so {#xscript-http}

Содержит [HTTP-блок](block-http-ov.md).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-js.so {#xscript-js}

Содержит [JavaScript-блок](block-js-ov.md).

**Входит в пакет**

[xscript-js](packages.md#xscript-js)


## xscript-json.so {#xscript-json}

Добавляет в [File-блок](block-file-ov.md) метод [loadJson](../appendices/block-file-methods.md#loadJson), предназначенный для загрузки JSON-данных.

Расширяет [HTTP-блок](block-http-ov.md) возможностью интерпретации JSON-данных.

Содержит XSL-функции [json-stringify](../appendices/xslt-functions.md#json-stringify) и [js-stringify](../appendices/xslt-functions.md#js-stringify).

Добавляет возможность использования метода `x:json` в XSL-шаблонах.

**Входит в пакет**

[xscript-json](packages.md#xscript-json)


## xscript-local.so {#local}

Содержит [Local-блок](block-local-ov.md) и [While-блок](block-while-ov.md).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-lua.so {#xscript-lua}

Содержит [Lua-блок](block-lua-ov.md).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-memcache.so {#xscript-memcache}

Кэш для хранения в памяти [результатов выполнения блоков](block-results-caching.md) и [выходных страниц](caching-ov.md).

Очищается при перезапуске XScript.

Локальный - на каждом front-end-е хранится своя копия кэша.

При одновременном использовании с [дисковым кэшом](modules.md#xscript-diskcache) приоритет обращения к кэшам для получения их элементов определяется порядком, в котором кэши подключены в конфигурационном файле.

На x64-серверах, где нет ограничений по памяти, имеет смысл использовать memcache вместо [дискового кэша](modules.md#xscript-diskcache), так как при большой нагрузке
 частые обращения к диску будут замедлять работу системы.

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-memcache.so](../appendices/config-params.md#xscript-memcache)

## xscript-mist.so {#xscript-mist}

Содержит [Mist-блок](block-mist-ov.md) и [XSL-функцию](../appendices/xslt-functions.md).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-mobile.so {#xscript-mobile}

Содержит [Mobile-блок](block-mobile-ov.md).

**Входит в пакет**

[xscript-mobile](packages.md#xscript-mobile)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-mobile.so](../appendices/config-params.md#xscript-mobile)

## xscipt-regional-units.so {#xscript-regional-units.so}

В модуле реализован [Regional-units-блок](block-regional-units-ov.md)

**Входит в пакет**

[xscript-regional-units](packages.md#xscript-regional-units)


## xscript-statistics.so {#xscript-statistics}

Включает сбор статистики работы XScript.

При установке пакетов [xscript-yandex-www](packages.md#xscript-yandex-www) и [xscript-yandex-www5](packages.md#xscript-yandex-www5) в директории `/usr/local/www/xscript` и `/usr/local/www5/xscript` соответственно помещается набор XML-файлов, которые содержат вызовы служебного блока, выдающего информацию о работе XScript.

Эти XML-файлы можно открыть из браузера (после настройки администратором в Lighttpd виртуального хоста для верстки) для просмотра собранной статистики.

Доступны следующие файлы со статистикой:

- [control-response-time.xml](../appendices/control-response-time.md) - статистика по временам ответов на запросы;
- [control-status-info.xml](../appendices/control-status-info.md) - информация о количестве потоков [threadpool](modules.md#xscript-thrpool)-а и [xscript-daemon](packages.md#xscript-daemon)-а, времени работы XScript;
- [control-tagged-block-cache-stat.xml](../appendices/control-tagged-block-cache-stat.md) - статистика работы [tagged-кэша](block-results-caching.md) для блоков;
- [control-tagged-page-cache-stat.xml](../appendices/control-tagged-page-cache-stat.md) - статистика [кэширования выходных страниц](caching-ov.md);
- [control-xml-cache-stat.xml](../appendices/control-xml-cache-stat.md) - статистика работы [XML- и XSL-кэшей](modules.md#xscript-xmlcache).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-thrpool.so {#xscript-thrpool}

Модуль, содержащий реализацию пула потоков (тредов) для асинхронного выполнения тредных блоков ([File-блока](block-file-ov.md), [HTTP-блока](block-http-ov.md), [CORBA-блока](block-corba-ov.md), [Auth-блока](block-auth-ov.md)).

Если данный модуль не подключен, все блоки будут выполняться в одном треде.

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-thrpool.so](../appendices/config-params.md#xscript-thrpool)

## xscript-tinyurl.so {#tinyurl}

Содержит [Tinyurl-блок](block-tinyurl-ov.md).


**Входит в пакет**

[xscript-tinyurl](packages.md#tinyurl)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-tinyurl.so](../appendices/config-params.md#xscript-tinyurl)

## xscript-xmlcache.so {#xscript-xmlcache}

Кэш разобранных XML- и XSL-файлов (в том числе перблочных XSL). Находится в памяти.

Для XML и XSL используются два разных кэша. Размер кэшей ограничен. Если в кэше не хватает места, для того чтобы сохранить новый элемент, из него удаляется элемент, к которому дольше всего не было обращений.

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-xmlcache.so](../appendices/config-params.md#xscript-xmlcache)


## xscript-xslt.so {#xscript-xslt}

Содержит набор [XSL-функций](../appendices/xslt-functions.md).

**Входит в пакет**

[xscript-standard](packages.md#xscript-standard)


## xscript-yandex.so {#xscript-yandex}

Содержит [Auth-блок](block-auth-ov.md), [Banner-блок](block-banner-ov.md), [Custom-morda-блок](block-custom-morda-ov.md), [Lang-detect-блок](block-lang-detect-ov.md) набор [XSL-функций](../appendices/xslt-functions.md), функциональность авторизации, поддержки [DPS](https://wiki.yandex-team.ru/dps) и проверки [Secret Key](secret-key.md).

**Входит в пакет**

[xscript-yandex](packages.md#xscript-yandex)

### Узнайте больше {#learn-more}
* [Настройки модуля xscript-yandex.so](../appendices/config-params.md#xscript-yandex)

## xscript-yandex-lua.so {#xscript-yandex-lua}

Содержит компоненты [Lua-блока](block-lua-ov.md), предназначенные для работы с Y-куками:

- классы: [xscript.ycookie.ys](../appendices/block-lua-ycookie-ys-methods.md), [xscript.ycookie.yp](../appendices/block-lua-ycookie-yp-methods.md), [xscript.ycookie.gp](../appendices/block-lua-ycookie-gp-methods.md), [xscript.ycookie.gpauto](../appendices/block-lua-ycookie-gpauto-methods.md), [xscript.ycookie.ygo](../appendices/block-lua-ycookie-ygo-methods.md);
- функции таблицы [xscript.ycookie](../appendices/block-lua-ycookie-methods.md).

**Входит в пакет**

[xscript-yandex](packages.md#xscript-yandex)


## xscript-yandex-sanitizer.so {#xscript-yandex-sanitizer}

При помощи алгоритма санитайзинга из Аркадии "очищает" HTML, удаляя из него опасные фрагменты кода. Возвращает nodeset, содержащий полученный в результате очистки XHTML.

Используется в HTTP-блоке.

При очистке выполняются следующие действия:

- Удаляются скрипты (\<srcipt\>, \<... onclick="..."\>, style="...expr(...)");
- Удаляются "плохие" стили (position: absolute, не инлайновые стили (\<style\>...\</style\>));
- Удаляются элементы class="...", id="...";
- Flash-контент с доменов, не входящих в список доменов, которым доверяет Яндекс, отправляется через прокси xss.yandex.net;
- Строится корректный xml (закрывает теги и т.д.).

Также содержит XSL-функцию [sanitize](../appendices/xslt-functions.md#sanitize).

**Входит в пакет**

[xscript-yandex-sanitizer](packages.md#xscript-yandex-sanitizer)


## xscript-uatraits.so {#xscript-uatraits}

Содержит [uatraits-блок](block-xscript-uatraits-ov.md), предназначенный для определения браузера по полю `User-Agent` http-заголовка.

**Входит в пакет**

[xscript-uatraits](packages.md#xscript-uatraits)

