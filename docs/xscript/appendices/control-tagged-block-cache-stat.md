# control-tagged-block-cache-stat.xml

Выдает статистику работы [tagged-кэша](../concepts/block-results-caching.md).

Структура файла:

```
<page>

   <block-cache>

      <!-- hit-ratio - отношение количество нахождений элементов в кэше к общему количеству обращений в кэш -->
      <block-cache-memory hit-ratio="0.3078722421162066">
          
          <!-- count - количество событий -->
          <!-- total - общее время запросов -->
          <!-- min - минимальное время запроса -->
          <!-- max - максимальное время запроса -->  
  
          <!-- hits - "попадания" в кэш (наличие в кэше нужного элемента) -->
          <hits count="10661" total="3517029" min="10" max="49795" avg="329"/>

          <!-- miss - "промахи" (отсутствие в кэше нужного элемента) -->
          <miss count="23967" total="329169" min="2" max="8508" avg="13"/>

          <!-- save - сохранение в кэше нового элемента -->
          <save count="23962" total="9807277" min="9" max="21675" avg="409"/>

          <!-- Блоки с самым плохим hit-ratio. -->
          <!-- Hit-ratio считается плохим, если он меньше значения, определенного в конфигурационном файле. -->
          <disgrace>

             <!-- info - информация о блоке: вид блока, имя серванта (для CORBA-блока), вызываемый метод, параметры. -->
             <element hit-ratio="0.2960910323683745" calls="16961" info="corba.yandex/fotki/web.id.getuserrecentimagessimple | Params: QueryArg(author), long(3), string(XXS) | Cache-time: undefined">

                <!-- Страница, с которой вызывается блок. -->
                <owner calls="11375">/usr/local/www5/lora/fotka-view.xml</owner>
                <owner calls="1665">/usr/local/www5/lora/context-top.xml</owner>
                <owner calls="1510">/usr/local/www5/lora/album-view.xml</owner>
                ...
             </element>
             ...
          </disgrace>

      </block-cache-memory>
   </block-cache>
</page>
```

Входит в пакет [xscript-yandex-www](../concepts/packages.md#xscript-yandex-www) или [xscript-yandex-www5](../concepts/packages.md#xscript-yandex-www5).

Устанавливается в директорию вёрстки (`/usr/local/www/xscript/` или `/usr/local/www5/xscript/`) и может быть просмотрен в окне браузера.

