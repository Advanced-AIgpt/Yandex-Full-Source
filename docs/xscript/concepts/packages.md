# Пакеты XScript

## libxscript {#libxscript}

Ядро XScript.

Состоит из компилятора XML, учитывающего правила языка XScript, среды для подключения и выполнения блоков и XSLT-шаблонизатора.

Загружает указанный в запросе XML, разбирает его и накладывает основной XSL. Если подключены дополнительные модули для обработки блоков, также будут выполнены блоки и наложен перблочный XSL. Если они не подключены, файл будет обрабатываться как обычный XML, без выполнения блоков.

Содержит набор [XSL-функций](../appendices/xslt-functions.md).

Осуществляет логирование.

**Содержит модули**:

\-

### Узнайте больше {#learn-more}
* [Настройки ядра XScript](../appendices/config-params.html#config-params__libxscript)


## libxscript-python {#libxscript-python}

Позволяет использовать движок XScript в языке Python.

Содержит три функции:

- `initialize` - инициализирует XScript;
- `renderBuffer` - обрабатывает XML, переданный в виде текстовой строки;
- `renderFile` - обрабатывает XML, загруженный из файла на диске.

Подробнее о перечисленных функциях читайте на [Вики](https://wiki.yandex-team.ru/IljaGolubcov/XscriptBinding).

**Содержит модули**:

\-


## xscript-corba {#xscript-corba}

Содержит функциональность для работы с CORBA.

**Содержит модули**:

[xscript-corba.so](modules.md#xscript-corba)


## xscript-daemon {#xscript-daemon}

Многопоточный FCGI-модуль веб-сервера. Использует ядро XScript для динамического формирования страниц.

Принимает HTTP-запрос от веб-сервера по протоколу FastCGI, разбирает его (получает HTTP-заголовки, тело сообщения) и передает в [ядро](packages.md#libxscript). Результат обработки запроса возвращается веб-серверу.

**Содержит модули**:

\-


## xscript-development {#development}

Режим "development".

**Содержит модули**:

[xscript-development.so](modules.md#xscript-development)


## xscript-geo {#xscript-geo}

Содержит функциональность геотаргетинга.

**Содержит модули**:

[xscript-geo.so](modules.md#xscript-geo)


## xscript-js {#xscript-js}

Содержит функциональность интерпретации JavaScript-кода.

Требует совместная установка с [libxscript](packages.md#libxscript) >= 5.73.31 и spidermonkey >= 1.8.5-1.yandex2, а также libnspr4-0d.

**Содержит модули**:

[xscript-js.so](modules.md#xscript-js)


## xscript-json {#xscript-json}

Содержит функциональность обработки JSON-данных.

**Содержит модули**:

[xscript-json.so](modules.md#xscript-json)


## xscript-mobile {#xscript-mobile}

Содержит функциональность для мобильных приложений.

**Содержит модули**:

[xscript-mobile.so](modules.md#xscript-mobile)


## xscript-standard {#xscript-standard}

Стандартная функциональность XScript.

**Содержит модули**:

- [xscript-http.so](modules.md#xscript-http);
- [xscript-file.so](modules.md#xscript-file);
- [xscript-local.so](modules.md#local);
- [xscript-lua.so](modules.md#xscript-lua);
- [xscript-mist.so](modules.md#xscript-mist);
- [xscript-xslt.so](modules.md#xscript-xslt);
- [xscript-thrpool.so](modules.md#xscript-thrpool);
- [xscript-xmlcache.so](modules.md#xscript-xmlcache);
- [xscript-diskcache.so](modules.md#xscript-diskcache);
- [xscript-memcache.so](modules.md#xscript-memcache);
- [xscript-statistics.so](modules.md#xscript-statistics).


## xscript-tinyurl {#tinyurl}

Функциональность "укорачивания" URL-ов.

**Содержит модули**:

[xscript-tinyurl.so](modules.md#tinyurl)


## xscript-utility {#xscript-utility}

Утилита [xscript-proc](xscript-tools-ov.md).

**Содержит модули**:

\-

### Узнайте больше {#learn-more}
* [Настройки утилиты xscript-proc](../appendices/config-params.html#xscript-offline)


## xscript-yandex {#xscript-yandex}

Содержит функциональность для внутреннего (только в компании Яндекс) использования.

**Содержит модули**:

[xscript-yandex.so](modules.md#xscript-yandex)


## xscript-yandex-sanitizer {#xscript-yandex-sanitizer}

Содержит функциональность санитайзинга.

**Содержит модули**:

[xscript-yandex-sanitizer.so](modules.md#xscript-yandex-sanitizer)


## xscript-yandex-www {#xscript-yandex-www}

Содержит набор [служебных XML-файлов](../appendices/control-response-time.md), использующих функциональность [xscript-statistics.so](modules.md#xscript-statistics). Файлы устанавливаются в директорию `/usr/local/www/xscript/` (старая директория верстки).

**Содержит модули**:

\-


## xscript-yandex-www5 {#xscript-yandex-www5}

Содержит набор [служебных XML-файлов](../appendices/control-response-time.md), использующих функциональность [xscript-statistics.so](modules.md#xscript-statistics). Файлы устанавливаются в директорию `/usr/local/www5/xscript/` (новая директория верстки).

**Содержит модули**:

\-


## xscript-botlist {#xscript-botlist}

Содержит файл `botlist.xml` со [списком](http://wiki.yandex-team.ru/passport/MDAforBOTs) подстрок HTTP-заголовка User-agent, на основании которых определяется, что зашедший на страницу пользователь является [роботом](bot-detect.md). Устанавливается в `/etc/xscript/`.

**Содержит модули**:

\-


## xscript-multiple-botlist {#xscript-multiple-botlist}

Содержит файл `botlist.xml` со [списком](http://wiki.yandex-team.ru/passport/MDAforBOTs) подстрок HTTP-заголовка User-agent, на основании которых определяется, что зашедший на страницу пользователь является [роботом](bot-detect.md). Устанавливается в `/etc/xscript-multiple/common/`.

**Содержит модули**:

\-


## xscript-cache-strategies {#cache-strategies}

Содержит набор стандартных стратегий кэширования. Устанавливается в `/etc/xscript/`.

**Содержит модули**:

\-


## xscript-multiple-cache-strategies {#cache-strategies-multiple}

Содержит набор стандартных стратегий кэширования. Устанавливается в `/etc/xscript-multiple/common/`.

**Содержит модули**:

\-


## xscript-regional-units {#xscript-regional-units}

Содержит функционал для представления скоростей, расстояний и температур в удобном для пользователя виде. Например, если пользователь находится в США, то на выходе он получит расстояние в привычных ему милях, футах и дюймах, а температуру в градусах Фаренгейта.

**Содержит модули:**

[xscipt-regional-units.so](modules.md#xscript-regional-units.so)


## xscript-uatraits, libuatraits, uatraits-data {#xscript-uatraits}

Обеспечивают функцию определения браузеров по полю `User-Agent` http-заголовка.

**Содержит модули:**

[xscript-uatraits.so](modules.md#xscript-uatraits)

