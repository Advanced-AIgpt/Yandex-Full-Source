# control-tagged-page-cache-stat.xml

Статистика кэширования [выходных страниц](../concepts/caching-ov.md).

Структура файла:

```
<page>

   <page-cache>
  
      <!-- hit-ratio - отношение количество нахождений элементов в кэше к общему количеству обращений в кэш -->    
      <page-cache-memory hit-ratio="0.6058199147952921">

         <!-- count - количество событий -->
         <!-- total - общее время запросов -->
         <!-- min - минимальное время запроса -->
         <!-- max - максимальное время запроса -->  
  
         <!-- hits - "попадания" в кэш (наличие в кэше нужного элемента) -->
         <hits count="8390" total="16092204" min="114" max="14754" avg="1918"/>
         <miss count="5459" total="34168" min="2" max="74" avg="6"/>
         <save count="5404" total="9702477" min="100" max="70231" avg="1795"/>

         <!-- Страницы с самым плохим hit-ratio. -->
         <!-- Hit-ratio считается плохим, если он меньше значения, определенного в конфигурационном файле. -->
         <disgrace>

            <!-- info - информация о запрошенной странице: её URL, путь в файловой системе. -->
            <element hit-ratio="0" calls="5" info="Url: http://weather.yandex.ru/33991/ | Filename: /usr/local/www5/weather/forecast.xml | Cache-time: 7200"/>
            <element hit-ratio="0.25" calls="4" info="Url: http://weather.yandex.ru/23804/ | Filename: /usr/local/www5/weather/forecast.xml | Cache-time: 7200"/>
            <element hit-ratio="0" calls="4" info="Url: http://weather.yandex.ru/7149/ | Filename: /usr/local/www5/weather/forecast.xml | Cache-time: 7200"/>
            ...

      </page-cache-memory>
   </page-cache>
</page>
```

Входит в пакет [xscript-yandex-www](../concepts/packages.md#xscript-yandex-www) или [xscript-yandex-www5](../concepts/packages.md#xscript-yandex-www5).

Устанавливается в директорию вёрстки (`/usr/local/www/xscript/` или `/usr/local/www5/xscript/`) и может быть просмотрен в окне браузера.

