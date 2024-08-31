# control-status-info.xml

Выдает информацию о количестве потоков threadpool-а и xscript-daemon-а, а также времени работы XScript-а.

Структура файла:

```
<page>
   <status-info>

      <!-- Количество потоков threadpool-а. -->
      <!-- count - Количество потоков, используемых в данный момент. -->
      <!-- peak - Максимальное количество одновременно занятых потоков. -->
      <!-- max - Максимально возможное количество потоков. -->
      <working-threads count="0" peak="21" max="50"/>

      <!-- Количество потоков xscript-daemon-а. -->
      <!-- count - Количество потоков, используемых в данный момент. -->
      <!-- peak - Максимальное количество одновременно занятых потоков. -->
      <!-- max - Максимально возможное количество потоков. -->
      <fcgi-workers count="4" peak="24" max="50"/>

      <!-- Время работы XScript. -->
      <!-- uptime - Время работы XScript в секундах. -->
      <uptime uptime="215596" days="2" hours="11" mins="53" secs="16"/>
   </status-info>
</page>
```

Входит в пакет [xscript-yandex-www](../concepts/packages.md#xscript-yandex-www) или [xscript-yandex-www5](../concepts/packages.md#xscript-yandex-www5).

Устанавливается в директорию вёрстки (`/usr/local/www/xscript/` или `/usr/local/www5/xscript/`) и может быть просмотрен в окне браузера.

