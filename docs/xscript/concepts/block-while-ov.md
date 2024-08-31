# While-блок

_While-блок_ позволяет реализовать цикл, который будет выполняться до тех пор, пока истинно условие, заданное в [\<guard\>](../reference/guard.md) или [\<guard-not\>](../reference/guard-not.md) блока.

Исполняемая часть цикла (скрипт, обрабатываемый XScript-ом) содержится в теге [\<root\>](../reference/root.md).

**Пример While-блока**:

```
<page xmlns:x="http://www.yandex.ru/xscript">
     <lua>f = 0;</lua>
     <x:while>
          <guard-not value="5">var</guard-not>
          <root>
              <iteration>
                  <mist method="dumpState/">
              </iteration>
              <lua>
                  f = f + 1;
                  xscript.state:setLong("var", f)
              </lua>
          </root>
     </x:while>
</page>
```

В приведенном примере цикл будет выполняться 5 раз, пока переменная `f` не станет равной 5.

В результате выполнения данного примера будет получен следующий результат:

```
<lua/>
<root>
     <iteration>
         ....
     </iteration>
     <lua/>
     <iteration>
         ....
     </iteration>
     <lua/>
     <iteration>
         ....
     </iteration>
     <lua/>
     <iteration>
         ....
     </iteration>
     <lua/>
     <iteration>
         ....
     </iteration>
     <lua/>
</root>
```

XML-выдача каждой итерации подклеивается к элементу \<root\>, являющемуся корневым в выдаче While-блока. Изменить имя корневого XML-элемента в ответе блока можно указав в теге `<root>` атрибут `name="нужное_имя_элемента"`. При этом в входном XML атрибут `name` присутствовать не будет.

```
<root name="page">
```

В теге `<while>` обязательно должно быть указано пространство имен _http://www.yandex.ru/xscript_. Если пространство имен не указано, тег \<while\> будет восприниматься как обычный XML-тег и While-блок не будет создан.

Кроме того, существует возможность задать пространство имен корневого XML-элемента в ответе блока. Для этого пространство имен должно быть определено в теге While-блока и указано в имени корневого элемента.

```
<x:while xmlns:g="http://www.ya.ru">
     <root name="g:page">
         ......
     </root>
</x:local>
```

Блок может выполняться асинхронно (если установлен атрибут [threaded="yes"](../appendices/attrs-ov.md#threaded)), а результаты его работы могут [кэшироваться](block-results-caching.md).

[Перблочное преобразование](per-block-transformation-ov.md) накладывается на результаты работы блока после выполнения всех итераций.

### Узнайте больше {#learn-more}
* [while](../reference/while.md)