# Прикладные средства XScript

Cреда XScript состоит из двух программ:
- `xscript-bin`: работает под веб-сервером постоянно, обрабатывая XML-страницы, отображаемые этим сервером;
- `xscript-proc`: утилита командной строки, "эмулирующая" работу XScript в offline-режиме.


## Утилита xscript-proc {#xscriptproc}

Утилита командной строки `xscript-proc` "эмулирует" работу XScript в offline-режиме без использования веб-сервера. Проверяет корректность XML- и XSL-файлов, обрабатывает и профилирует запрос.

Результаты обработки запроса выводятся в стандартный поток и поток ошибок. В стандартный поток передаются результирующий XML-файл или HTML-файл и информация от XSLT-профайлера. В поток ошибок – ошибки валидации файла и обработки запроса.

#### Конфигурирование утилиты

Для правильного конфигурирования `xscript-proc` в [файле настроек](../appendices/config.md) необходимо определить следующие параметры:

- [/xscript/offline/root-dir](../appendices/config-params.md#root-dir) - корневая директория верстки по умолчанию. Может быть переопределена с помощью параметров запуска утилиты `--root-dir` или `--docroot`;
- [xscript/logger-factory](../appendices/config-params.md#file) - настройки файлового логгера. В частности необходимо указать `xscript/logger-factory/logger/file` - путь к лог-файлу (желательно в своей home-директории);
- [/xscript/modules/module/path](../appendices/config-params.md#module) - путь к подключаемому модулю XScript;
    - при подключении модуля `xscript-thrpool.so` необходимо указать количество потоков для асинхронного выполнения блоков ([/xscript/pool-workers](../appendices/config-params.md#pool-workers));
    - при подключении модуля `xscript-diskcache.so` (дисковый tagged-кэш) необходимо указать путь к директории, в которой будет храниться кэш ([/xscript/tagged-cache-disk/root-dir](../appendices/config-params.md#cache-root-dir)).
    
- [/xscript/auth](../appendices/config-params.md#need-yandexuid-cookie) - параметры авторизации.

#### Запуск утилиты

```
xscript-proc file | url [options]
```

Принимает путь к файлу (`file`) или URL ресурса (`url`).

Путь файла может быть указан как со схемой, так и без нее.

##### Без указания схемы

Если путь к файлу указан без схемы, то при наличии прямого слеша в начале строки, будет считаться, что путь указан абсолютный, а если слеш отсутствует - относительно рабочей директории утилиты.

Например, при запуске утилиты из директории `/home/user/project` с указанными ниже параметрами будет обработан файл `/home/user/project/xml/index.xml`.

```
/usr/bin/xscript-proc --config=/home/user/xscript_custom.conf xml/index.xml
```

##### Схемы http:// и https:// 

Позволяет добавить в переменные окружения и HTTP-заголовки хост и порт.

Например, при запуске утилиты с указанными ниже параметрами будут добавлены переменные окружения `SERVER_NAME="www.ya.ru"`, `SERVER_PORT="8081"`, `HTTPS="ON"`, и HTTP-заголовок `Host`="www.ya.ru". При этом будет обработан файл `/home/user/project2/xml/index.xml`.

```
/usr/bin/xscript-proc --config=/home/user/xscript_custom.conf --docroot=/home/user/project2 
https://www.ya.ru:8081/xml/index.xml
```

##### Схема dps://

Позволяет указать путь к файлу относительно прописанной в конфигурационном файле директории `dps-root`.

Допустим, в конфигурационном файле указан dps-root `/var/cache/dps/stable`. Тогда при запуске утилиты с указанными ниже параметрами будет обработан файл `/var/cache/dps/stable/index.xml`.

```
/usr/bin/xscript-proc --config=/home/user/xscript_custom.conf
dps://index.xml
```

##### Схема docroot://

Позволяет указать корневую директорию для документов.

Например, при запуске утилиты с указанными ниже параметрами будет обработан файл `/home/user/project2/xml/index.xml`.

```
/usr/bin/xscript-proc --config=/home/user/xscript_custom.conf
--docroot=/home/user/project2 docroot://xml/index.xml
```

#### Параметры утилиты (options)

```
--config=file
--docroot=<value> | --root-dir=<value>
--header=<value> [ .. --header=<value> ]
--profile | --norman = [text|xml]
--stylesheet=<value>
--dont-apply-stylesheet | --dont-apply-stylesheet=all
--dont-use-remote-call
--noout
```

- `config`: путь к файлу настроек; опциональный параметр. Если данный параметр не указан, загружается конфигурационный файл `/etc/xscript/offline.conf`;
- `header`: HTTP-заголовки;
    ```
    --header="User-Agent: Mozilla/5.0"
    ```
    
- `dont-apply-stylesheet`: позволяет отключить XSLT-наложения. Если этот ключ указан без значения, то будет отключено основное XSLT-наложение. Если ему присвоено значение “all”, будут отключены как основное, так и перблочные наложения;
- `stylesheet=<value>`: позволяет переопределить основное XSLT-наложение. При его использовании `xscript-proc` игнорирует инструкцию `<xml-stylesheet>` в обрабатываемом XML-файле и использует указанный XSL в качестве основного XSL-наложения.
- `dont-use-remote-call`: ключ, позволяющий отключить удаленные вызовы CORBA-компонентов;
- `profile` или `norman`: ключ, позволяющий вывести информацию от XSLT-профайлера о работе основного и перблочных xslt-наложений. Если значение ключа равно "text" или пустой строке, то используется текстовый вывод, если "xml" – вывод в виде XML-дерева. Текстовое представление профайлера формируется из XML-представления посредством XSLT-наложения, указанного в файле настроек в теге `<xslt-profile-path>` блока `<xscript|offline>`. По умолчанию используется наложение `/etc/share/xscript-proc/profile.xsl`. Таким образом, пользователь утилиты при желании может самостоятельно изменять представление выдачи профайлера;
- `root-dir` или `docroot`: корневая директория верстки. Имеет более высокий приоритет по сравнению с соответствующей настройкой в конфигурационном файле;
- `noout`{#noout}: отключает вывод результата. При этом будут выводится сообщения об ошибках и результаты работы XSLT-профайлера.

#### Примеры

Допустим, нужно проверить файл `/usr/local/www5/devel/test.xml`. Сделать это можно следующими способами:

```
./xscript-proc --profile http://devel.yandex.ru/devel/test.xml

./xscript-proc --config=/home/user/xscript_custom.conf http://devel.yandex.ru/devel/test.xml

./xscript-proc --config=/home/user/xscript_custom.conf
/usr/local/www5/devel/test.xml
```

### Узнайте больше {#learn-more}
* [Настройки утилиты xscript-proc в конфигурационном файле XScript](../appendices/config-params.md#xscript-offline)