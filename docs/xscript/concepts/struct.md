# Список пакетов и модулей XScript

## Основные пакеты и модули {#main}

XScript имеет модульную структуру. Ниже перечислены все модули XScript и пакеты, в которых они поставляются.

#|
|| Пакеты | Описание пакета | Модули | Описание модуля || 
|| [libxscript](packages.md#libxscript) | Ядро XScript. | - | - || 
|| [xscript-daemon](packages.md#xscript-daemon) | Многопоточный FCGI-модуль веб-сервера. | - | - || 
|| [xscript-standard](packages.md#xscript-standard) | Стандартная функциональность XScript. |

1. [xscript-http.so](modules.md#xscript-http)
1. [xscript-file.so](modules.md#xscript-file)  
1. [xscript-local.so](modules.md#local) 
1. [xscript-lua.so](modules.md#xscript-lua)  
1. [xscript-mist.so](modules.md#xscript-mist)   
1. [xscript-xslt.so](modules.md#xscript-xslt) 
1. [xscript-thrpool.so](modules.md#xscript-thrpool)   
1. [xscript-xmlcache.so](modules.md#xscript-xmlcache) 
1. [xscript-diskcache.so](modules.md#xscript-diskcache)  
1. [xscript-memcache.so](modules.md#xscript-memcache) 
1. [xscript-statistics.so](modules.md#xscript-statistics) 

| 

1. [HTTP-блок](block-http-ov.md).
1. [File-блок](block-file-ov.md).  
1. [Local-блок](block-local-ov.md). 
1. [Lua-блок](block-lua-ov.md). 
1. [Mist-блок](block-mist-ov.md) и [XSL-функция](../appendices/xslt-functions.md).   
1. Набор [XSL-функций](../appendices/xslt-functions.md). 
1. Пул потоков (тредов) для асинхронного выполнения тредных блоков.   
1. Кэш разобранных XML- и XSL-файлов. 
1. Кэш для хранения на диске [результатов выполнения блоков](block-results-caching.md) и [выходных страниц](caching-ov.md).  
1. Кэш для хранения в памяти [результатов выполнения блоков](block-results-caching.md) и [выходных страниц](caching-ov.md). 
1. Модуль, включающий сбор статистики работы XScript. 

|| 
|| [xscript-yandex](packages.md#xscript-yandex) | Функциональность для внутреннего (только в компании Яндекс) использования. | 
[xscript-yandex.so](modules.md#xscript-yandex) | [Auth-блок](block-auth-ov.md), [Banner-блок](block-banner-ov.md), [Custom-morda-блок](block-custom-morda-ov.md), набор [XSL-функций](../appendices/xslt-functions.md), функциональность авторизации, поддержки [DPS](https://wiki.yandex-team.ru/dps) и проверки [Secret Key](secret-key.md). || 
|| [xscript-corba](packages.md#xscript-corba) | Функциональность для работы с CORBA. | [xscript-corba.so](modules.md#xscript-corba) | [CORBA-блок](block-corba-ov.md). || 
|| [xscript-geo](packages.md#xscript-geo) | Функциональность геотаргетинга. | [xscript-geo.so](modules.md#xscript-geo) | [Geo-блок](block-geo-ov.md) и набор [XSL-функций](../appendices/xslt-functions.md). || 
|| [xscript-js](packages.md#xscript-js) | Функциональность интерпретации JavaScript-кода. | [xscript-js.so](modules.md#xscript-js) | [JavaScript-блок](block-js-ov.md). || 
|| [xscript-mobile](packages.md#xscript-mobile) | Функциональность для мобильных приложений. | [xscript-mobile.so](modules.md#xscript-mobile) | [Mobile-блок](block-mobile-ov.md). || 
|| [xscript-tinyurl](packages.md#tinyurl) | Функциональность "укорачивания" URL-ов. | [xscript-tinyurl.so](modules.md#tinyurl) | [Tinyurl-блок](block-tinyurl-ov.md). || 
|| [xscript-yandex-sanitizer](packages.md#xscript-yandex-sanitizer) | Функциональность санитайзинга. | [xscript-yandex-sanitizer.so](modules.md#xscript-yandex-sanitizer) | "Очищает" HTML, удаляя из него опасные фрагменты кода. || 
|| [xscript-utility](packages.md#xscript-utility) | Утилита [xscript-proc](xscript-tools-ov.md). | - | - || 
|| [libxscript-python](packages.md#libxscript-python) | Позволяет использовать движок XScript в языке Python. | - | - || 
|| [xscript-development](packages.md#development) | Режим "development". | [xscript-development.so](modules.md#xscript-development) | Модуль, включающий режим "development". || 
|| [xscript-yandex-www](packages.md#xscript-yandex-www) | Набор служебных XML-файлов для сбора статистики. | - | - || 
|| [xscript-yandex-www5](packages.md#xscript-yandex-www5) | Набор служебных XML-файлов для сбора статистики. | - | - || 
|| [xscript-botlist](packages.md#xscript-botlist) | Список [роботов](bot-detect.md). | - | - || 
|| [xscript-multiple-botlist](packages.md#xscript-multiple-botlist) | Список [роботов](bot-detect.md). | - | - || 
|| [xscript-cache-strategies](packages.md#cache-strategies) | Набор [стандартных стратегий кэширования](page-cache-strategies.md). | - | - || 
|| [xscript-multiple-cache-strategies](packages.md#cache-strategies-multiple) | Набор [стандартных стратегий кэширования](page-cache-strategies.md). | - | - || 
|| [xscript-regional-units](packages.md#xscript-regional-units) | Преобразование величин | [xscipt-regional-units.so](modules.md#xscript-regional-units.so) | - || 
|| [xscript-uatraits, libuatraits, uatraits-data](packages.md#xscript-uatraits) | Определение браузера по полю `User-Agent` http-заголовка | [xscript-uatraits.so](modules.md#xscript-uatraits) | - ||
|#

Cтандартное окружение продакшн-сервера с XScript включает в себя пакеты [libxscript](packages.md#libxscript), [xscript-daemon](packages.md#xscript-daemon) и [xscript-standard](packages.md#xscript-standard).  


## Специальные пакеты и модули {#special}

Некоторые проекты Яндекса самостоятельно разрабатывают модули XScript, содержащие функциональность, специфическую для проекта.

В настоящий момент (декбрь 2009) существуют следующие специальные пакеты XScript:

- xscript-mda-ya -функциональность, разработанная для проекта Я.ру. Содержит модуль xscript-mda-ya.so - авторизация на Я.ру;
- libyandex-maps-xsltext - функциональность, разработанная командой Яндекс.Карт. Содержит модуль [libyandex-maps-xsltext.so](https://wiki.yandex-team.ru/JandeksKarty/development/fordevelopers/libs/xsltext) - блок, XSL-функция и XSL-элемент для симплификации полилиний, заданных в формате YMAPSML;
- libyandex-maps-ipregion - функциональность, разработанная командой Яндекс.Карт. Содержит модуль [libyandex-maps-ipregion.so](https://wiki.yandex-team.ru/JandeksKarty/development/fordevelopers/libs/ipregion) - блок, позволяющий определить регион по IP-адресу.

### Узнайте больше {#learn-more}
* [Параметры файла настроек](../appendices/config-params.html)