# Файл настроек

Конфигурационный файл XScript содержит настройки [ядра и модулей XScript](../concepts/struct.md). Его имя задаётся при старте XScript (с помощью параметра `--config`).

Ниже приведен пример файла настроек XScript и даны комментарии к нему. Более подробное описание параметров файла настроек см. в разделе [Параметры файла настроек](config-params.md).

Доступ к значению параметра файла настроек можно получить с помощью переменной типа [VHostArg](../concepts/parameters-matching-ov.md#vhostex).

```
<?xml version="1.0" ?>
<!-- $Id: xscript.conf,v 1.10 2007/03/23 09:35:05 highpower Exp $ -->
<xscript>

 <!-- Параметры лога: -->
 <logger-factory>
   <logger>
      <id>default</id>
      <level>info</level>
      <type>file</type>
      <file>/var/log/xscript/xscript.log</file>
      <print-thread-id>yes</print-thread-id>
      <read>all</read>
   </logger>
 </logger-factory>


 <!-- Настройки TCP/IP: -->
 <endpoint>

    <!-- Количество входящих соединений в очереди, ожидающих обработки XScript-ом;
         Только для экспертов! Изменяйте это значение, только если хорошо представляете, к чему это приведет: -->
    <backlog>50</backlog>

    <!-- Сокет для общения с web-сервером: -->
    <socket>/tmp/xscript.sock</socket>

 </endpoint>


  <!-- Номер альтернативного порта: 
       если запрос пришел на этот порт, не накладывать основной xsl;
       по умолчанию используется порт 8080 -->
  <alternate-port>8080</alternate-port>
 
  <!-- Номер порта, при запросе на который не накладывается ни основной xsl, ни перблочные преобразования; 
       по умолчанию данная функциональность работает на порту 8079 --> 
  <noxslt-port>8079</noxslt-port>

  <!-- Настройки кэширования: -->
  <script-cache>
     <buckets>10</buckets>
     <bucket-size>200</bucket-size>
  </script-cache>
  <stylesheet-cache>
     <buckets>10</buckets> <!-- то же самое, только для XSL -->
     <bucket-size>200</bucket-size>
  </stylesheet-cache>

 <pidfile>/var/run/xscript.pid</pidfile> <!-- PID -->


 <!-- Пул потоков для выполнения асинхронных методов: -->
 <pool-workers>50</pool-workers>

 <!-- Пул потоков для выполнения скриптов: -->
 <fastcgi-workers>50</fastcgi-workers>

 <!-- Список ключей для подписывания ссылок -->
 <yandex-redirect>

    <!-- Номер ключа, используемого по умолчанию. Если не установлен, 
         используется нулевой ключ (нумерация ключей начинается с нуля) -->
    <default-key-no>3</default-key-no>

    <!-- Основной URL сервиса Клик-демон. Его можно переопределить в 
         переменной окружения веб-сервера  -->
    <redirect-base-url>http://clck2.yandex.ru/redir/</redirect-base-url>
       <keys>
          <key>aR3zxenEu6+rgV84g2qoHQ==</key>
          <key>ayFVMGPqmKf4pZ0rnsGMGQ==</key>
          <key>24ntBqnEvWw3jHAvnKJEvA==</key>
          <key>hqYz6+YZIl4AfHzMUGl/xA==</key>
          ...
       </keys>

 </yandex-redirect>

 <!-- Настройки для генерации подписанных ссылок для Ресайзера изображений: -->
 <images-resizer>
     <!-- Ключ -->
     <secret>cb3bc5fb1542f6aab0c80eb84a17bad9</secret>
     <!-- Адрес Ресайзера изображений -->
     <base-url>http://resize.yandex.net</base-url>
 </images-resizer>

 <!-- Дополнительные подключаемые модули - не рекомендуется редактировать: -->
 <modules>
     <module id="logger">
        <path>/usr/lib/xscript/xscript-logger.so</path>
     </module>
     <module id="thread-pool">
        <path>/usr/lib/xscript/xscript-thrpool.so</path>
     </module>
     <module id="xml-factory">
        <path>/usr/lib/xscript/xscript-xmlcache.so</path>
     </module>
     <module id="http-block">
        <path>/usr/lib/xscript/xscript-http.so</path>
     </module>
     <module id="mist-block">
        <path>/usr/lib/xscript/xscript-mist.so</path>
     </module>
     <module id="local-block">
        <path>/usr/lib/xscript/xscript-local.so</path>
     </module>
     <module id="tinyurl">
        <path>/usr/lib/xscript/xscript-tinyurl.so</path>
     </module>
     <module id="regional-units-block">
        <path>/usr/lib/xscript/xscript-regional-units.so</path>
     </module>
     <module id="uatraits">
        <path>/usr/lib/xscript/xscript-uatraits.so</path>
     </module>
 </modules>

 <auth>
    <!-- Базовый адрес сервиса Blackbox -->
    <blackbox-url>http://blackbox.yandex.net/blackbox</blackbox-url>

    <!-- Базовый адрес сервиса Яндекс.Паспорт (на этот адрес выполняется редирект для авторизации пользователя) -->
    <auth-path>http://passport.yandex.ru/passport</auth-path>

    <!-- Базовый адрес для получения защищенной авторизации -->
    <sauth-path>https://passport.yandex.ru/passport</sauth-path>
 </auth>
 
 <!-- Стратегии кэширования XML-страниц -->
 <page-cache-strategies>
    <strategy id="my_cache_strategy">
       <query sort="no">var1,var2,var3</query>
       <cookie>service_id1,service_id2</<cookie>
       <cookie-my>15,17</cookie-my>
        <region default="100">23,578</region>
    </strategy>
      ...
 </page-cache-strategies>

</xscript>
```

### Узнайте больше {#learn-more}
* [Требования для системного администратора](../concepts/requirements-admin.md)
* [Параметры файла настроек](../appendices/config-params.md)