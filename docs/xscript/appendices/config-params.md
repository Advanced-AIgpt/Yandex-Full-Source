# Параметры файла настроек

В приведенной ниже таблице описаны параметры файла настроек. В поле "Путь в файле" указан путь в XML-файле к тегу параметра в виде `тег/ещё какой-то тег/.../тег данного параметра`. При этом из пути исключен корневой тег `<xscript>`.

Например, описанный ниже путь `alternate-port` в XML-файле будет соответствовать такой последовательности тегов:

```
<xscript>
   <alternate-port>8080</alternate-port>
</xscript>
```

Доступ к значению параметра файла настроек можно получить с помощью переменной типа [VHostArg](../concepts/parameters-matching-ov.md#vhostex).

#|
|| Пакет/модуль | Путь в файле | Описание | Значение по умолчанию | Тип/ допустимые значения ||
|| [_libxscript_](../concepts/packages.md) | alternate-port | Порт, на котором будет отключено [основное XSL-преобразование](../concepts/general-transformation-ov.md). | 8080 | Целое число ||
|| [_libxscript_](../concepts/packages.md) | auth/bots/bot | Идентификатор робота (подстрока HTTP-заголовка User-agent). | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | endpoint/backlog | Длина очереди запросов в веб-сервере, ожидающих обработку XScript-ом. | - | Целое число ||
|| [_libxscript_](../concepts/packages.md) | endpoint/port | Порт FastCGI-сокета. | - | Целое число ||
|| [_libxscript_](../concepts/packages.md) | endpoint/socket{#socket} | Путь к FastCGI-сокету. | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | fastcgi-workers | Количество FastCGI-worker-ов. | - | Целое число ||
|| [_libxscript_](../concepts/packages.md) | file-block/max-invoke-depth | Максимально допустимая длина цепочки вызовов метода [invoke](block-file-methods.md#invoke) File-блока. | 10 | Целое число ||
|| [_libxscript_](../concepts/packages.md) | input-buffer | Длина в байтах входного буфера FastCGI-потока. | 4096 | Целое число ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/file{#file} | Путь к лог-файлу файлового логгера | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/read | Параметр, определяющий права на чтение лог-файла файлового логгера. `all` - права на чтение есть у любого пользователя; `group` - группа пользователей, имеющих полномочия на чтение лог-файла; `user` - имя пользователя, у которого есть права на чтение лог-файла. | all | `all`, `group`, `user` ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/crash-on-errors | Если параметру присвоено значение «yes», XScript прерывает работу при неудачной попытке открыть лог-файл. | yes | `yes`, `no` ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/id | Имя логгера | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/type | Тип логгера: syslog или файл (file). | syslog | `syslog`, `file` ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/level | Уровень логирования | info | `debug`, `info`, `warn`, `error`, `crit` ||
l|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/print-request-id | Если параметру присвоено значение <q>yes</q>, в лог записываются идентификаторы запросов с. Идентификатор - простой счетчик с автоинкрементом. | no | yes, no ||
|| [_libxscript_](../concepts/packages.md) | logger-factory/logger/print-thread-id | Если параметру присвоено значение "yes", в лог будет записываться id потока, в котором выполняется XScript. | no | `yes`, `no` ||
|| [_libxscript_](../concepts/packages.md) | logger/ident | Идентификатор процесса XScript для логгера по умолчанию (syslog). | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | logger/level | Уровень логирования для логгера по умолчанию (syslog). | info | `debug`, `info`, `warn`, `error`, `crit` ||
|| [_libxscript_](../concepts/packages.md) | modules/module/path {#module} | Путь к подгружаемому модулю XScript. | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | modules/module/logger | Имя логгера, специфичного для данного модуля XScript. | default | Строка ||
|| [_libxscript_](../concepts/packages.md) | noxslt-port | Порт, на котором будет отключено как основное, так и перблочное [XSL-наложение](../concepts/per-block-transformation-ov.md). | 8079 | Целое число ||
|| [_libxscript_](../concepts/packages.md) | output-buffer | Длина в байтах выходного буфера FastCGI-потока. | 4096 | Целое число ||
|| [_libxscript_](../concepts/packages.md) | page-cache-strategies/strategy | [Стратегия кэширования](../concepts/page-cache-strategies.md) XML-страницы. | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | page-cache-strategies/strategy/cookie | Перечисленные через запятую имена кук запроса, которые будут участвовать в формировании ключа кэширования.<br/><br/>См. [Стратегии кэширования XML-страниц](../concepts/page-cache-strategies.md). | - | Строка ||
|| [_libxscript_](../concepts/packages.md) |  page-cache-strategies/strategy/cookie-my | Перечисленные через запятую блоки куки my, которые будут участвовать в формировании ключа.<br/><br/>См. [Стратегии кэширования XML-страниц](../concepts/page-cache-strategies.md). | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | page-cache-strategies/strategy/query | Перечисленные через запятую имена параметров запроса, которые будут участвовать в формировании ключа кэширования.<br/><br/>См. [Стратегии кэширования XML-страниц](../concepts/page-cache-strategies.md). | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | page-cache-strategies/strategy/region | Перечисленные через запятую идентификаторы регионов, которые будут участвовать в формировании ключа кэширования.<br/><br/>См. [Стратегии кэширования XML-страниц](../concepts/page-cache-strategies.md). | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | pidfile | Путь к pid-файлу XScript-а. | - | Строка ||
|| [_libxscript_](../concepts/packages.md) | xslt/max-depth | Максимально допустимая длина цепочки вызовов XSLT-шаблонов. | 300 | Целое число ||
|| утилита [_xscript-proc_](../concepts/xscript-tools-ov.md){#xscript-offline} | offline/root-dir{#root-dir} | Корневая директория по умолчанию для offline-процессора (используется утилитой [xscript-proc](../concepts/xscript-tools-ov.md) и в биндингах). | /usr/local/www | Строка ||
|| утилита [_xscript-proc_](../concepts/xscript-tools-ov.md){#xscript-offline} | offline/xslt-profile-path | Путь к XSL-наложению для XSLT-профайлера offline-процессора ([xscript-proc](../concepts/xscript-tools-ov.md)). | /usr/share/xscript-proc/profile.xsl | Строка ||
|| [_xscript-statistics_](../concepts/modules.md) | statistics/tagged-cache/output-size | Количество элементов tagged кэша с наихудшим hit-ratio при выводе статистики. | 20 | Целое число ||
|| [_xscript-statistics_](../concepts/modules.md) | statistics/tagged-cache/hit-ratio-level | Уровень фильтрации по hit-ratio при выводе статистики | 0.3 | Число с плавающей точкой ||
|| [_xscript-statistics_](../concepts/modules.md) | statistics/tagged-cache/refresh-time | Частота старта процедуры по удалению из статистики редко вызываемых элементов tagged кэша из статистики (в секундах). | 60 | Целое число ||
|| [_xscript-statistics_](../concepts/modules.md) | statistics/tagged-cache/max-idle-time | Время (в секундах), по истечение которого элемент tagged кэша считается устаревшим для статистики, если к нему не было обращений. | 600 | Целое число ||
|| [_xscript-diskcache_](../concepts/modules.md){#xscript-diskcache} | tagged-cache-disk/root-dir{#cache-root-dir} | Путь к директории дискового кэша. | - | Строка ||
|| [_xscript-diskcache_](../concepts/modules.md){#xscript-diskcache} | tagged-cache-disk/min-cache-time | Минимальное время кэширования для дискового кэша (в секундах). | 6 | Целое число ||
|| [_xscript-diskcache_](../concepts/modules.md){#xscript-diskcache} | tagged-cache-disk/no-cache | Запрещает кэширование в дисковом кэше результата обработки страницы (`page`) или/и блоков (`block`). | - | `page` или/и `block`, разделенные запятой или пробелом (например «page, block») ||
|| [_xscript-memcache_](../concepts/modules.md){#xscript-memcache} | tagged-cache-memory/pools | Количество пулов в memory-кэше. | 16 | Целое число ||
|| [_xscript-memcache_](../concepts/modules.md){#xscript-memcache} | tagged-cache-memory/pool-size | Размер пула в memory-кэше. | 128 | Целое число ||
|| [_xscript-memcache_](../concepts/modules.md){#xscript-memcache} | tagged-cache-memory/min-cache-time | Минимальное время кэширования для memory-кэша (в секундах). | 5 | Целое число ||
|| [_xscript-memcache_](../concepts/modules.md){#xscript-memcache} | tagged-cache-memcache/no-cache | Запрещает кэширование в памяти результатов обработки страницы (`page`) или блоков (`block`). | - | `page` или/и `block`, разделенные запятой или пробелом (например «page, block») ||
|| [_xscript-thrpool_](../concepts/modules.md){#xscript-thrpool} | pool-workers {#pool-workers} | Количество потоков в thread-пуле. | - | Целое число ||
|| [_xscript-xmlcache_](../concepts/modules.md){#xscript-xmlcache} | script-cache/buckets | Количество сегментов в XML-кэше. | 10 | Целое число ||
|| [_xscript-xmlcache_](../concepts/modules.md){#xscript-xmlcache} | script-cache/bucket-size | Количество элементов в сегменте XML-кэша. | 200 | Целое число ||
|| [_xscript-xmlcache_](../concepts/modules.md){#xscript-xmlcache} | script-cache/refresh-delay | Время задержки (в секундах) в обновлении элемента XML-кэша после изменения XML-файла. | 5 | Целое число ||
|| [_xscript-xmlcache_](../concepts/modules.md){#xscript-xmlcache} | stylesheet-cache/buckets | Количество сегментов в XSL-кэше. | 10 | Целое число ||
|| [_xscript-xmlcache_](../concepts/modules.md){#xscript-xmlcache} | stylesheet-cache/bucket-size | Количество элементов в сегменте XSL-кэша. | 200 | Целое число ||
|| [_xscript-xmlcache_](../concepts/modules.md){#xscript-xmlcache} | stylesheet-cache/refresh-delay | Время задержки (в секундах) в обновлении элемента XSL-кэша после изменения XSL-файла. | 5 | Целое число ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/account-info-fields {#account-info-fields} | Список полей, которые могут быть запрошены методом Auth-блока [get_all_info](block-auth-methods.md#get_all_info). | fio, sex, birth_date, charset, mail_format, email, nickname, reg_date, city, country | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/auth-path | URL для обращения к сервису Яндекс.Паспорт. | http://passport.yandex.ru/passport | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/blackbox-atime-level | Максимальное значение access-time к Blackbox (в миллисекундах), по истечение которого выдавать в лог XScript-а записывается сообщение уровня `WARN`. | 500 | Целое число ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/blackbox-retries {#blackbox-retries} | Количество повторных попыток получить ответ от Blackbox-а при разборе куки Session_ID. | 2 | Целое число ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} |  auth/blackbox-sleep-timeout {#blackbox-sleep-timeout} | Время ожидания между попытками получить ответ от Blackbox. | 100 | Целое число ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/blackbox-url | URL для обращения к Blackbox-у. | http://blackbox.yandex.net/blackbox | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/domains/domain/value | Домен, для которого необходимо определить специфическое правило редиректа `redirect`. | - | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} |  auth/domains/domain/redirect | URL, на который будет происходить редирект с указанного домена `value`. | - | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/need-yandexuid-cookie{#need-yandexuid-cookie} | Указывает, требуется ли установка куки _yandexuid_. | false | Булев тип данных ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/root-domain | Корневой домен. | yandex.ru | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/sauth-path | URL сервиса Яндекс.Паспорт для защищенной авторизации пользователя. | https://passport.yandex.ru/passport | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | auth/sids {#sids} | Список SID-ов, информация по которым может быть получена с помощью метода Auth-блока [get_all_info](block-auth-methods.md#get_all_info). | 2, 3, 4, 5, 6, 8, 13, 14, 15, 16, 20, 23, 25, 26, 29, 30, 31, 34, 35, 36, 37, 39, 41, 42, 44, 46, 47, 48 | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | dps/dps-root {#dps-root} | Путь к корневой директории DPS. Параметр должен иметь такое же значение, что и `dps/storage/cache/path`. Этот параметр используется Xscript'ом для поиска файлов в локальной схеме DPS. | /var/cache/dps/stable | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | dps/storage/cache/path | Путь к локальному кэшу DPS. Параметр должен иметь такое же значение, что и `dps/dps-root`. В отличие от `dps/dps-root`, значение этого параметра используется модулем `libdps` для сохранения в локальный кэш, если кэширование включено. | /var/cache/dps/stable/ | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | dps/storage/read | URL сервера DPS. | http://dps-proxy.corba.yandex.net/ | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | dps/no-caching | Не использовать кэширование в `libdps`. При значении параметра <q>true</q>, каждый запрос к `libdps` вызывает закачку файла с сервера, вместо чтения из локального кэша. | false | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | dps/cache/timeout | Таймаут для кэша в секундах. Параметр задает период, в течение которого файл в локальном DPS-кэше считается актуальным. По истечении заданного времени с помощью `libdps` производится повторная загрузка файла. Значение параметра игнорируется, если параметр `dps/no-caching` имеет значение <q>true</q>. | 30 | Целое число ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | dps/version | Версия файла, требуемая при загрузке с сервера DPS. Наиболее часто используются значения `latest` и `stable`, также может быть использовано явное указание версии. | stable | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | images-resizer/secret | Ключ для генерации ссылок для [Ресайзера изображений](http://wiki.yandex-team.ru/resizer). | cb3bc5fb1542f6aab0c80eb84a17bad9 | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | images-resizer/base-url | Адрес [Ресайзера изображений](http://wiki.yandex-team.ru/resizer). | http://resize.yandex.net | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | output-encoding {#output-encoding} | Кодировка по умолчанию, в которой страница выдается пользователю. | windows-1251 | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | yandex-redirect/default-key-no {#default-key-no} | Ключ по умолчанию для x:yandex-redirect(). | 0 | Целое число ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | yandex-redirect/keys/key | Ключ для x:yandex-redirect(). | - | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | yandex-redirect/redirect-base-url {#redirect-base-url} | URL по умолчанию для [x:yandex-redirect()](xslt-functions.md#yandex-redirect). | http://clck.yandex.ru/redir/ | Строка ||
|| [_xscript-yandex_](../concepts/packages.md){#xscript-yandex} | yandex/secret-key-salt {#yandex-secret-key-sault}| Фраза, которая будет использоваться при генерации секретного токена. Ограничений по формату нет. | (пустая строка) | Строка ||
|| [_xscript-corba_](../concepts/packages.md){#xscript-corba} | auth/auth-factory-path | Id серванта AuthFactory. | Yandex/Auth/Factory.id | Строка ||
|| [_xscript-corba_](../concepts/packages.md){#xscript-corba} | auth/auth-factory-timeout | Максимальное время соединения с AuthFactory (в миллисекундах). | 1000 | Целое число ||
|| [_xscript-geo_](../concepts/packages.md){#xscript-geo} | geodata/local-proxies | Признак включения/отключения алгоритма определения оптимального сервера выдачи статики. При значении `yes` метод [`getLocalProxyBaseUrl`](block-geo-methods.md#get_local_proxy_baseurl) будет пытаться определить сервер выдачи, при значении `no` — возвратит сообщение об ошибке. | no | `yes`, `no` ||
|| [_xscript-geo_](../concepts/packages.md){#xscript-geo} | geodata/local-proxies-base-urls-path | Путь к файлу, определяющему региональные серверы выдачи статики для проектов. | /var/cache/regional/regions-static.xml | Строка ||
|| [_xscript-geo_](../concepts/packages.md){#xscript-geo} | geodata/local-proxies-path | Путь к файлу с геоданными для определения региона по IP. | /var/cache/geobase/geodata-local.bin | Строка ||
|| [_xscript-geo_](../concepts/packages.md){#xscript-geo} | geodata/path | Путь к файлу с данными для работы Geo-блока. | - | Строка ||
|| [_xscript-geo_](../concepts/packages.md){#xscript-geo} | geodata/search-enabled | Разрешение поиска методом Geo-блока `search_regions`. | <q>no</q> | <q>yes</q>, <q>no</q> ||
|| [_xscript-geo_](../concepts/packages.md){#xscript-geo} | geodata/timeout | Временной интервал (в секундах), через который обновляется файл с данными для работы [Geo-блока](../concepts/block-geo-ov.md). | - | Целое число ||
|| [_xscript-http_](../concepts/modules.md){#xscript-http-config} | /xscript/http-block/checked-headers | Проверка заголовков при добавлении | 1 | 0 или 1 ||
|| [_xscript-http_](../concepts/modules.md) | /xscript/http-block/checked-query-params | Проверка параметров при добавлении | 1 | 0 или 1 ||
|| [_xscript-http_](../concepts/modules.md) | /xscript/http-block/keep-alive | Включение/отключение поддержания HTTP-соединения. | <q>yes</q> | <q>yes</q>, <q>no</q> ||
|| [_xscript-http_](../concepts/modules.md) | /xscript/http-block/load-entities | Загружать сущности из xml-ответа. | <q>yes</q> | <q>yes</q>, <q>no</q> ||
|| [_xscript-mobile_](../concepts/packages.md){#xscript-mobile} | mobiledata/path | Путь к файлу с данными о мобильных телефонах. | /var/cache/phonedetect/index | Строка ||
|| [_xscript-mobile_](../concepts/packages.md){#xscript-mobile} | mobiledata/timeout | Временной интервал (в секундах), через который обновляется файл с данными о мобильных телефонах. | 120 | Целое число ||
|| [_xscript-mobile_](../concepts/packages.md){#xscript-mobile} | mobiledata/path | Путь к файлу с данными о мобильных телефонах. | /var/cache/phonedetect/index | Строка ||
|| [_xscript-tinyurl_](../concepts/modules.md){#xscript-tinyurl} | tinyurl/url | Адрес HTTP-сервиса "укорачивания" URL-ов. Для целей разработки можно использовать http://tinyurl-test.yandex.ru/tiny. | http://tinyurl.yandex.net/tiny | Строка ||
|#

### Узнайте больше {#learn-more}
* [Файл настроек](../appendices/config.md)
* [Требования для системного администратора](../concepts/requirements-admin.md)