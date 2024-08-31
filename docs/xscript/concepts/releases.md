# Релизы

В этом разделе содержится информация об опубликованных релизах документа. Для каждого релиза указан номер, дата публикации, краткое содержание изменений, вошедших в релиз.

#### Релиз 2.20

_Июнь 2015_

Актуализировано описание [тега \<http\>](../reference/http.md) и [глобальных настроек http-модуля](../appendices/config-params.md#xscript-http-config).

#### Релиз 2.19

_Июль 2014_

Добавлено описание [метода search_regions Geo-блока](../appendices/block-geo-methods.md#search_regions).

#### Релиз 2.18

_Ноябрь 2013_
Добавлено [описание новых конфигурационных параметров](../appendices/config-params.md) модуля `xscript-yandex`: `dps/storage/cache/path`, `dps/storage/read`, `dps/no-caching`, `dps/cache/timeout`, `dps/version`, `yandex/secret-key-salt`.

#### Релиз 2.17

_Февраль 2013_
Добавлено описание двух новых методов [transformToString](../appendices/block-js-xscript-methods.md#transformtostring) и [xsltProcess](../appendices/block-js-xscript-methods.md#xsltprocess) [js-блока](block-js-ov.md).
Добавлено описание возможности доступа к контейнеру [LocalArgs](block-local-ov.md#localargs) из тегов [\<xslt\>](../reference/xslt-tag.md) и [\<xpointer\>](../reference/xpointer-tag.md).

#### Релиз 2.16
_Май 2012 года_
Уточнено описание методов [json-strigify](../appendices/xslt-functions.md#json-stringify) и [js-stringify](../appendices/xslt-functions.md#js-stringify). Эти методы теперь вопринимают элементы `json`, содержащие данные в формате JSON. Версия пакета `xscript-json` >= 5.73-25

Добавлено описание метода [crc32](../appendices/block-js-xscript-methods.md#crc32) пространства имён `xscript` JavaScript-блока. Версия пакета `xsciprt-js` >= 5.73-16.

Добавлено описание функции [crc32](../appendices/block-lua-other-methods.md#crc32) таблицы `xscript` Lua-блока. Версия пакета `xscript-standard` >= 5.73.35-2.

Добавлено описание xslt функции [crc32](../appendices/xslt-functions.md#crc32). Версия пакета `libxscript` >= 5.73.35-2.

Уточнено описание метода [get_fields](../appendices/block-auth-methods.md#get_fields) Auth-блока, который теперь возвращает дополнительное поле `authid`, позволяющее идентифицировать сессию пользователя даже в случае обновления куки.

Добавлено описание блока [uatraits](block-xscript-uatraits-ov.md) и функций модуля JavaScript-блока [xscript.uatraits](../appendices/block-js-xscript-uatraits-module.md), которые позволяют определить браузер пользователя по полю `User-Agent` http-заголовка.

Добавлено описание функций модуля JavaScript-блока [xsript.langdetect](../appendices/block-js-xscript-langdetect-module.md), которые позволяют определить регион и язык пользователя. Новые функции доступны в `xscript-yandex` >= 5.73–62. 

Добавлено описание двух новых методов [cookie2language](../appendices/block-lang-detect-methods.md#cookie2language) и [language2cookie](../appendices/block-lang-detect-methods.md#language2cookie) блока `lang-detect`. Новые методы доступны в `xscript-yandex` >= 5.73–62. 

Добавлено описание функций модуля JavaScript-блока [xscript.cache](../appendices/block-js-xscript-cache-module.md), которые позволяют кэшировать объекты в оперативной памяти. Новые методы доступны в `xscript-standard >= 5.73.41`.

Добавлено описание нового [настроечного](../appendices/config-params.md) параметра XScript `/xscript/logger-factory/logger/print-request-id`. Если значение этого параметра <q>yes</q> — в журнал записываются идентификаторы запросов. Новая функциональность доступна в пакетах `libxscript >= 5.73.39-3, xscript-js >= 5.73-31`.

Добавлено описание нового свойства `parse` блока [http](../reference/http.md). В зависимость от значения этого атрибута получаемые данные интрепретируются как текст или как документ XML. Требуется `libxscript >= 5.73.40-1`.

Добавлено описание псевдообъекта [xscript.HttpRequest](../appendices/block-js-xscript-httprequest-object.md) JavaScript-блока. Требуется `xscript-js >= 5.73-32`.

Добавлено описание модуля JavaScript-блока [xscript.sanitize](../appendices/block-js-xscript-sanitize-module.md), который использует алгоритм санитайзинга из Аркадии для <q>очистки</q> html. Новая функция доступна при подключении `xscript-yandex-sanitizer >= 5.73–1`.

Добавлено описание простого [секретного ключа](secret-key.md), который генерируется без использования авторизационной информации, а также метода его получения с помощью xslt-функции [get-easy-secret-key](../appendices/xslt-functions.md#get-easy-secret-key). Новая функция доступна в `xscript-yandex >= 5.73-38`.

Добавлено описание модулей [xscript.secretkey](../appendices/block-js-xscript-secretkey-module.md) и [xscript.yandex](../appendices/block-js-xscript-yandex-module.md) JavaScript-блока. Новая функциональность доступна в `xscript-yandex >= 5.73–60`.

Добавлено описание модуля [xscript.geo](../appendices/block-js-xscript-geo-module.md) JavaScript-блока. Новая функциональность доступна в `xscript-geo >= 5.73–62`.
#### Релиз 2.15

_Сентябрь 2011 года_


Добавлено описание JavaScript-блока, позволяющего интерпретировать JavaScript-код. Блок поставляется в пакете [xscript-js](packages.md#xscript-js), который требуется устанавливать совместно с [libxscript](packages.md#libxscript]) >= 5.73.31 и spidermonkey >= 1.8.5-1.yandex2, а также libnspr4-0d.

Добавлено описание метода Lua-блока [write](../appendices/block-lua-response-methods.md#write), предназначенного для формирования HTTP-ответа <q>с нуля</q>.

Уточнены правила преобразования [XML→JSON](../appendices/xml-to-json.md) для XML-элементов, имеющих дочерние элементы с одинаковым именем.

#### Релиз 2.14

_Июль 2011 года_


Добавлено описание пакета [xscript-regional-units](packages.md#xscript-regional-units) и содержащегося в нём модуля.

#### Релиз 2.13

_Июль 2011 года._

Добавлено описание нового тега [query-param](../reference/query-param.md), предназначенного для задания параметров HTTP-запроса, выполняемого с помощью HTTP-блока.

Описан сокращенный синтаксис задания параметров контейнерного типа в тегах [param](../reference/param.md), [xslt-param](../reference/xslt-param.md), [query-param](../reference/query-param.md).

Изменено описание метода [get_fields](../appendices/block-auth-methods.md#get_fields) Auth-блока, который теперь позволяет получить данные из социального профиля пользователя. Добавлено описание метода [get_bulk_fields](../appendices/block-auth-methods.md#get_bulk_fields), возвращающего информацию о пользователях из таблиц сервиса Яндекс.Паспорт.
 
 Для получения информации о каждом пользователе используется метод get_fields.

Изменено описание условий выполнения блоков, задаваемых с помощью тегов [guard](../reference/guard.md) и [guard-not](../reference/guard-not.md) и одноименных атрибутов.

#### Релиз 2.12

_Июнь 2011 года._

Поддержка JSON:

- Добавлен раздел [Поддержка JSON](../appendices/json.md), описывающий преобразования [JSON→XML](../appendices/json-to-xml.md)и [XML→JSON](../appendices/xml-to-json.md).
- Новый модуль [xscript-json.so](modules.md#xscript-json.so), входящий в пакет [xscript-json](packages.md#xscript-json).
- Добавлено описание метода File-блока [loadJson](../appendices/block-file-methods.md#loadJson), предназначенного для загрузки JSON-данных из файла.
- Добавлено [описание возможности интерпретации JSON-данных, загружаемых методами HTTP-блока](../appendices/block-http-methods.md).
- Добавлено описание XSL-функций [json-stringify](../appendices/xslt-functions.md#json-stringify) и [js-stringify](../appendices/xslt-functions.md#js-stringify), преобразующих XML-элементы в формат JSON.

Добавлено описание тега [header](../reference/header.md), позволяющего выставлять/изменять заголовки HTTP-запроса в методах HTTP-блока.

Обновлено описание [File-блока](block-file-ov.md), описан [способ возвращения бинарного контента из файла](../tasks/how-to-return-binary-file.md) с помощью метода [loadBinary](../appendices/block-file-methods.md#loadbinary).

#### Релиз 2.11

_Май 2011 года._

Изменился набор полей, которые можно получить с помощью методов [Geo-блока](block-geo-ov.md).

Обновлено описание процесса обработки [XScript-страницы](request-handling-file.md). Описан процесс [асинхронной обработки блока](request-handling-file.md#async) в отдельном потоке.

Изменено описание [процесса обработки XScript-блока](block-handling-ov.md). Исправлен ряд ошибок.

Добавлено описание способа [предоставления доступа к родительскому объекту Request из Local-блока](block-local-ov.md#request_access). Начиная с версии **libxscript 5.73.21-5**.

Добавлено описание блока Lang-detect, предназначенного для автоматического определения языка отображения страницы и определения релевантных пользователю языков. Начиная с версии **xscript-yandex 5.73-21**.

Добавлено описание метода [erase_state](../appendices/block-mist-methods.md#erase_state) Mist-блока и метода [xscript.state:erase](../appendices/block-lua-state-methods.md#erase) Lua-блока, позволяющих удалить переменную из контейнера State. Начиная с версии **xscript 5.73.9-1**.

#### Релиз 2.10

_Апрель 2011 года._

Добавлено описание [методов Geo-блока](../appendices/block-geo-methods.md), расширяющих возможности блока по определению местоположения пользователя: [is_in](../appendices/block-geo-methods.md#is_in), [is_yandex](../appendices/block-geo-methods.md#is_yandex), [get_geo_location](../appendices/block-geo-methods.md#get_geo_location).

#### Релиз 2.9

_Февраль 2011 года._

Добавлено описание XSLT-функции [x:domain](../appendices/xslt-functions.md#domain) , выделяющей домен указанного уровня из названия хоста или URL-а.

Добавлено описание функций, производящих punycode-кодирование и декодирование доменных имен. XSLT-функции: [punycode-domain-encode](../appendices/xslt-functions.md#punycode-domain-encode), [punycode-domain-decode](../appendices/xslt-functions.md#punycode-domain-decode). Функции Lua-блока: [punycodeDomainEncode](../appendices/block-lua-other-methods.md#punycodeDomainEncode), [punycodeDomainDecode](../appendices/block-lua-other-methods.md#punycodeDomainDecode).

Добавлено описание компонентов Lua-блока, предназначенных для работы с Y-куками: классы [xscript.ycookie.ys](../appendices/block-lua-ycookie-ys-methods.md), [xscript.ycookie.yp](../appendices/block-lua-ycookie-yp-methods.md), [xscript.ycookie.gp](../appendices/block-lua-ycookie-gp-methods.md), [xscript.ycookie.gpauto](../appendices/block-lua-ycookie-gpauto-methods.md), [xscript.ycookie.ygo](../appendices/block-lua-ycookie-ygo-methods.md), [функции таблицы xscript.ycookie](../appendices/block-lua-ycookie-methods.md).

#### Релиз 2.8

_Ноябрь 2010 года._

Добавлено описание метода Geo-блока [getLocalProxyBaseUrl](../appendices/block-geo-methods.md#get_local_proxy_baseurl), предназначенного для определения наиболее подходящего регионального сервера выдачи статики по IP. Параметры, определяющие работу метода, указываются в элементе `geodata`[конфигурационного файла XScript](../appendices/config-params.md#xscript-geo). Начиная с версии **xscript-geo 5.73-3**.

Добавлено описание блока [Local-program](block-local-program-ov.md), предназначенного для определения принадлежности IP-адреса к сети, подключенной к [региональной программе Яндекса (<q>Локальная сеть</q>)](http://local.yandex.ru/). Начиная с версии **xscript-geo 5.73-7**.

#### Релиз 2.7

_Октябрь 2010 года._

Добавлено описание [стандартных стратегий кэширования](page-cache-strategies.md).

#### Релиз 2.6

_Сентябрь 2010 года._

Изменился формат XML-сериализации [контейнера с метаинформацией](meta.md). Начиная с версии **xscript 5.71.26-1**.

Изменился набор полей, которые можно получить с помощью методов [Geo-блока](block-geo-ov.md). Начиная с версии **xscript-geo 5.71-13**.

В Lua-блоке появились инструкция [return](block-lua-ov.md) и новые функции: [cookie.httpOnly](../appendices/block-lua-cookie-methods.md#httponly), [localargs.getAll](../appendices/block-lua-localargs-methods.md#getall), [request.getHeaders](../appendices/block-lua-request-methods.md#getheaders), [state.getAll](../appendices/block-lua-state-methods.md#getall). Начиная с версии **xscript 5.71.27-1**.

Изменились входные параметры методов Mist-блока [dumpState](../appendices/block-mist-methods.md#dump_state), [echoCookies](../appendices/block-mist-methods.md#echo_cookies), [echoHeaders](../appendices/block-mist-methods.md#echo_headers), [echoProtocol](../appendices/block-mist-methods.md#echo_protocol) и [echoRequest](../appendices/block-mist-methods.md#echo_request). Начиная с версии **xscript 5.71.27-1**.

Добавлено описание механизма [определения запросов от роботов](bot-detect.md).

#### Релиз 2.5

_Август 2010 года._

Появилась возможность получать перблочное XSL-преобразование из контейнера State с помощью тега [\<xslt\>](../reference/xslt-tag.md). Начиная с версии **xscript 5.71.8-1**.

#### Релиз 2.4

_Июль 2010 года._

Добавлен атрибут [except](page-cache-strategies.md#except) тега \<query\>. Начиная с версии **xscript 5.70.65-1**.

Добавлен механизм получения [метаинформации](meta.md) о вызове блока. Начиная с версии **xscript 5.71.3-1**.

Добавлены Lua-методы [xscript.state:setTable](../appendices/block-lua-state-methods.md#set-table), [xscript.state:getTypedValue](../appendices/block-lua-state-methods.md#get-typed-value), [xscript.localargs:getTypedValue](../appendices/block-lua-localargs-methods.md#get-typed-value), [xscript.dateparse](../appendices/block-lua-other-methods.md#dateparse). Начиная с версии **xscript 5.71.3-1**.

Появился [Tinyurl-блок](block-tinyurl-ov.md). Начиная с версии **xscript-tinyurl 5.71-1**.

#### Релиз 2.3

_Май 2010 года._

Новый тег [\<xpointer\>](../reference/xpointer-tag.md). Начиная с версии **xscript 5.70-57**.

Появилась возможность задавать [пространство имен корневого элемента](block-local-ov.md#root-node-namespace) XML-ответа Local-блока. Начиная с версии **xscript 5.70-61**.

Новый метод Geo-блока [get_native_region](../appendices/block-geo-methods.md#get_native_region). Начиная с версии **xscript-geo 5.70-12**.

Появился [While-блок](block-while-ov.md). Начиная с версии **xscript 5.70.67-2**.

#### Релиз 2.2

_Март 2010 года_
.

В Lua-блоке добавлен метод [request.getQueryArgs](../appendices/block-lua-request-methods.md#get-query-args) и удален метод buildQueryString. Начиная с версии **xscript 5.70-25**.

Появился [Local-блок](block-local-ov.md). Начиная с версии **xscript 5.70-37**.

Новый параметр default метода [get](../appendices/block-lua-state-methods.md#get) объекта xscript.state в Lua-блоке. Начиная с версии **xscript 5.70-37**.

Модуль [xscript-development.so](modules.md#xscript-development) вынесен из пакета xscript-standard в пакет xscript-development. Начиная с версии **xscript-development 5.70-35**.

Прекращено использование типа PageRandom и атрибута page-random-max тега \<xscript\>.

Добавлен механизм описания [стратегий кэширования](page-cache-strategies.md) страниц. Начиная с версий **libxscript 5.70-22**,
 **xscript-yandex 5.70-8** и
 **xscript-geo 5.70-5**.

Добавлены методы File-блока [loadBinary](../appendices/block-file-methods.md#loadbinary) и [loadText](../appendices/block-file-methods.md#loadtext). Начиная с версии **xscript 5.70-40**.

Добавлена XSL-функция [gen-resizer-url-crop](../appendices/xslt-functions.md#gen-resizer-url-crop). Начиная с версии **xscript-yandex 5.70-12**.

#### Релиз 2.1

_Январь 2010 года._

Добавлена XSL-функция [gen-resizer-url](../appendices/xslt-functions.md#gen-resizer-url). Начиная с версии **xscript-yandex 5.70-2**.

Добавлен метод Lua-блока buildQueryString. Начиная с версии **xscript 5.70-2**.

#### Релиз 2.0

_Декабрь 2009 года._

Добавлена новая информация о [структуре](struct.md) и [настройках](../appendices/config-params.md) XScript.

Изменена структура документа.

#### Релиз 1.6

_Ноябрь 2009 года._

Добавлена новая XSL-функция [if](../appendices/xslt-functions.md#if). Начиная с версии **ядра 5.69-18**.

Добавлен новый метод Lua-блока [strsplit](../appendices/block-lua-other-methods.md#strsplit). Начиная с версии **xscript 5.69-14**.

Добавлен новый параметр XSL-функции [get-protocol-arg](../appendices/xslt-functions.md#get-protocol-arg). Начиная с версии **xscript 5.69-13**.

Появилась возможность наложения [XPointer](../appendices/xpointer.md) на результаты работы блока, вызванного из основного или перблочного XSL-преобразования. Начиная с версии **xscript 5.69-13**.

Появилась возможность запрашивать значение параметра конфигурационного файла с помощью метода Lua-блока [getVHostArg](../appendices/block-lua-other-methods.md#vhostarg), параметра типа [VHostArg](parameters-matching-ov.md#vhostarg) и XSL-функции [get-vhost-arg](../appendices/xslt-functions.md#get-vhost-arg). Начиная с версии **xscript 5.69-13**.

Добавлены новые методы Lua-блока [base64encode](../appendices/block-lua-other-methods.md#base64encode) и [base64decode](../appendices/block-lua-other-methods.md#base64decode). Начиная с версии **xscript 5.69-10**.

Добавлены новые допустимые [поля в Geo-блоке](../appendices/block-geo-methods.md#tz_abbr). Начиная с версии **xscript-geo 5.69-2**.

#### Релиз 1.5

_Октябрь 2009 года._

Добавлено описание [Mobile-блока](block-mobile-ov.md). Начиная с версии **xscript-mobile 5.68-3**.

Добавлено описание XSL-функций ([get-secret-key](../appendices/xslt-functions.md#get-secret-key) и [check-secret-key](../appendices/xslt-functions.md#check-secret-key)) и параметров ([SecretKey](parameters-matching-ov.md#secret-key) и [CheckSecretKey](parameters-matching-ov.md#check-secret-key)) для генерации и проверки [секретного ключа](secret-key.md). Начиная с версии **xscript-yandex 5.68-4**.

Добавлены новые допустимые значения входных параметров метода [get_fields](../appendices/block-auth-methods.md#get_fields) Auth-блока.

#### Релиз 1.4

_Сентябрь 2009 года._

Изменения в методах [set_state_by_request](../appendices/block-mist-methods.md#set_state_by_request) и [set_state_by_request_urlencoded](../appendices/block-mist-methods.md#set_state_by_request_urlencoded). Начиная с версии **xscript-yandex 5.67-1**.

Новый метод Lua-блока [setExpireTimeDelta](../appendices/block-lua-response-methods.md#set_expire_time_delta). Начиная с версии **ядра XScript 5.68-1**.

Новый метод Lua-блока [drop](../appendices/block-lua-state-methods.md#drop). Начиная с версии **ядра XScript 5.68-4**.

Добавлена возможность использовать VHostArg и ProtocolArg в [\<guard\>](../reference/guard.md) и [\<guard-not\>](../reference/guard-not.md). Начиная с версии **ядра XScript 5.68-5**.

Изменение значения по умолчанию атрибута delim тега [\<xpath\>](../reference/xpath.md). Начиная с версии **ядра XScript 5.68-6**.

Появилась возможность указывать [неймспейс](../appendices/attrs-ov.md#xmlns), который должен использоваться при обработке тега \<xpath\> и атрибута xpointer. Начиная с версии **ядра XScript 5.68-6**.

#### Релиз 1.3 

Август 2009 года.

Новые методы Auth-блока: [set_state_email_by_domain](../appendices/block-auth-methods.md#set_state_email_by_domain) и [set_state_mail_db](../appendices/block-auth-methods.md#set_state_mail_db).

#### Релиз 1.2

Июль 2009 года.

Новая XSLT-функция [x:mist](../appendices/xslt-functions.md#mist).

Новый атрибут HTTP-блока [print-error-body](../appendices/attrs-ov.md#print-error-body).

Изменения политики [обработки ошибок XSL](error-diag-ov.md).

Добавлена информация о [способах адресации файлов](../appendices/file-address.md) в XScript.

Новые методы Lua-блока: [attachStylesheet](../appendices/block-lua-other-methods.md#attach-stylesheet), [dropStylesheet](../appendices/block-lua-other-methods.md#drop-stylesheet), [skipNextBlocks](../appendices/block-lua-other-methods.md#skip-next-blocks), [stopBlocks](../appendices/block-lua-other-methods.md#stop-blocks), [suppressBody](../appendices/block-lua-other-methods.md#suppress-body).

Возвращена функциональность [xi:fallback](../reference/xi-fallback.md).

В утилиту xscript-proc добавлен ключ [noout](xscript-tools-ov.md#noout).

#### Релиз 1.1

Июнь 2009 года.

Появилась возможность учитывать блоки куки my в рассчете ключа кэширования страницы.

Новые методы Auth-блока: [get_bulk_logins](../appendices/block-auth-methods.md#get_bulk_logins), [get_bulk_names](../appendices/block-auth-methods.md#get_bulk_names), [set_state_username](../appendices/block-auth-methods.md#set_state_username).

#### Релиз 1.0

Май 2009 года. Первая версия документа.

