# control-xml-cache-stat.xml

Показывает статистику работы [XML- и XSL-кэшей](../concepts/modules.md#xscript-xmlcache).

Структура файла:

```
<page>

<!-- XML- и XSL-кэши разбиты на подкэши, для каждого из которых показаны: -->
<!-- elements - количество элементов, в данный момент находящихся в кэше. -->
<!-- max-elements - максимально возможное количество элементов в кэше (определяется в конфигурационном файле). -->
<!-- usage - среднее число запросов к элементу за время его жизни в кэше. -->

   <!-- Статистика работы XML-кэша -->
   <script-cache>
      <xml-storage elements="35" max-elements="48" usage="1255.557142857143"/>
      <xml-storage elements="41" max-elements="48" usage="700.9009900990097"/>
      <xml-storage elements="37" max-elements="48" usage="872.9615384615383"/>
      ...
   </script-cache>

   <!-- Статистика работы XSL-кэша -->
   <stylesheet-cache>
      <xml-storage elements="16" max-elements="32" usage="215.9375"/>
      <xml-storage elements="13" max-elements="32" usage="823.6153846153846"/>
      <xml-storage elements="16" max-elements="32" usage="5793.25"/>
      ...
   </stylesheet-cache>
</page>
```

Входит в пакет [xscript-yandex-www](../concepts/packages.md#xscript-yandex-www) или [xscript-yandex-www5](../concepts/packages.md#xscript-yandex-www5).

Устанавливается в директорию вёрстки (`/usr/local/www/xscript/` или `/usr/local/www5/xscript/`) и может быть просмотрен в окне браузера.

