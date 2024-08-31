# Требования для системного администратора

XScript состоит из следующих пакетов:
- _libxscript_ - ядро XScript;
- _xscript-daemon_ - демон XScript;
- _xscript-utility_ - утилита [xscript-proc](xscript-tools-ov.md);
- _xscript-standard_ - подключаемые модули XScript:
    - _xscript-mist.so_ - Mist-блок;
    - _xscript-http.so_ - HTTP-блок;
    - _xscript-file.so_ - File-блок;
    - _xscript-lua.so_ - Lua-блок;
    - _xscript-thrpool.so_ - пул потоков (thread pool);
    - _xscript-xmlcache.so_ - кэш разобранных XML-файлов;
    - _xscript-diskcache.so_ - дисковый tagged кэш;
    - _xscript-memcache.so_ - tagged кэш в памяти;
    - _xscript-memcached.so_ - tagged кэш в memcached;
    - _xscript-development.so_ - режим development;
    - _xscript-statistics.so_ - библиотека для сбора статистики работы Xscript;
    
- _xscript-yandex-sanitizer_ - yandex-санитайзер; содержит модуль _xscript-yandex-sanitizer.so_;
- _xscript-yandex_ - технологии Яндекса, Banner-блок и блок Custom-morda;
- _xscript-corba_ - CORBA-технологии (CORBA-блок и Auth-блок); содержит модуль _xscript-corba.so_;
- _xscript-geo_ - предоставляет информацию о гео-административной структуре территории Земли и обеспечивает определение местонахождения пользователя в этой структуре; содержит модуль _xscript-geo.so_.
- _xscript-mda-ya_ - поддержка мультидоменной авторизации; содержит модуль _xscript-mda-ya.so_.

В каждом пакете находятся определенные модули, которые подключаются [в конфигурационном файле](../appendices/config.md).

XScript работает с веб-сервером _lighttpd_. Общение между XScript-ом и веб-сервером происходит по протоколу _FastCGI_ через сокет, для настройки которого необходимо указать путь до него в параметре файла настроек [\<xscript|endpoint|socket\>](../appendices/config-params.md#socket), а также в настройке виртуального хоста веб-сервера, как показано в приведенном ниже примере:

```
$HTTP["host"] == "devel.yandex.ru" {
         ssl.engine = "enable"
         server.document-root = "/usr/local/www/devel"

         fastcgi.server = (
                 ".xml" => ((
                         "socket" => "/tmp/xscript.sock",
                         "check-local" => "enable",
                         "allow-x-send-file" => "enable"
                 ))
         )
}
```

### Узнайте больше {#learn-more}
* [Обработка запроса](../concepts/xscript-functionality.md)
* [Файл настроек](../appendices/config.md)
* [Параметры файла настроек](../appendices/config-params.md)
* [Переменные окружения веб-сервера, обрабатываемые XScript-ом](../appendices/env-var.md)