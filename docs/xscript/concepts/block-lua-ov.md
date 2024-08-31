# Lua-блок

В XScript 5 реализована возможность писать код на языке Lua. Такой код должен размещаться внутри Lua-блока.

Подробную информацию о Lua можно найти в [Справочном руководстве по языку Lua 5.1](http://www.lua.ru/doc/) (на русском языке) и книге [Programming in Lua](http://www.lua.org/pil/).

**Пример Lua-блока**:

```
<lua>
    -- Определим переменную
    hello = 'Hello world'

    -- Выведем значение этой переменной
    print(hello)
        
    -- Определим функцию 
    function sayHello()
        print ('Hello world again')
    end
              
    -- Вызов функции        
    sayHello()
</lua>
```

Функции, определённые в Lua-блоке, доступны и в других блоках этого скрипта. Это даёт возможность писать в XScript Lua-библиотеки.

```
<lua>
    function foo(key, value)
        xscript.state:setString(key, value)
    end
<lua>
 
<lua>
    foo('bar', 'baz')
<lua>
```

Включение кода Lua в XScript-документ осуществляется таким образом, чтобы не нарушалась грамматика XML. Это означает, что при необходимости использовать в Lua-коде специальные символы, следует либо заменять их на сущности, либо размещать код в разделах `<![CDATA] ... ]]>`.

```
<lua>
    s = '&lt;myelement/&gt;'
    print(s)
</lua>

<lua>
     <![CDATA[
      print('<myelement/>')
    ]]>
</lua>
```

Lua-блок позволяет:

- использовать стандартные функции языка Lua;
- обращаться к базовым объектам Lua, являющихся <q>обертками</q> для внутренних объектов XScript.
- создавать объекты классов, разработанных для использования в XScript;
- использовать дополнительные функции lua, предназначенных для использования в XScript.


## Компоненты Lua для XScript {#lua-objects}

Все программные компоненты Lua, разрабатываемые для использования в XScript, принадлежат пространству имён xscript. К таким компонентам относятся базовые объекты, классы и функции.

### Основные компоненты Lua для XScript {#lua-objects}

#|
|| [xscript.state](../appendices/block-lua-state-methods.md)

Доступ к содержимому контейнера State | [xscript.localargs](../appendices/block-lua-localargs-methods.md)

Доступ к содержимому контейнера LocalArgs | [xscript.cookie](../appendices/block-lua-cookie-methods.md)

Работа с произвольными куками ||
|| 
[xscript.request](../appendices/block-lua-request-methods.md)

Доступ к HTTP-запросу | [xscript.meta](../appendices/block-lua-meta.md)

Доступ к содержимому контейнера с метаинформацией о вызове блока | xscript.ycookie

Работа с Y-куками ||
|| 
[xscript.response](../appendices/block-lua-response-methods.md)

Доступ к HTTP-ответу | [xscript.logger](../appendices/block-lua-logger-methods.md)

Запись сообщений в лог | ||
|#

_Базовыми объектами_ будем называть экземпляры классов Lua, создаваемые и инициализируемые и XScript'ом самостоятельно во время обработки страницы. Базовые объекты создаются автоматически, поэтому соответствующие им классы не имеют публичных конструкторов.

Кроме базовых объектов существует набор _классов Lua, предназначенных для использования в XScript_. Объекты этих классов создаются разработчиком обычным образом — с помощью конструктора.

Часть Lua-функционала не имеет смысла реализовывать в виде классов. В этом случае используются _функции, которые помещаются в таблицу xscript_.

Таблица `xscript` содержит как функции, так и вложенные таблицы. Это позволяет (с точки зрения синтаксиса) использовать для функций то же пространство имен, что и для классов. Например, пространству имен `xscript.ycookie` принадлежит как объект `xscript.ycookie.ys`, так и функция `xscript.ycookie.getValue`.

**Пространство имен xscript**

- Базовый объект [xscript.state](../appendices/block-lua-state-methods.md) — предоставляет доступ к содержимому контейнера [State](state-ov.md). Частично заменяет методы [Mist-блока](block-mist-ov.md).
- Базовый объект [xscript.request](../appendices/block-lua-request-methods.md) — предоставляет доступ к HTTP-запросу.
- Базовый объект [xscript.response](../appendices/block-lua-response-methods.md) — предоставляет доступ к HTTP-ответу, позволяет изменять HTTP-ответ произвольным образом.
- Базовый объект [xscript.localargs](../appendices/block-lua-localargs-methods.md) — предоставляет доступ к содержимому контейнера [LocalArgs](block-local-ov.md#localargs).
- Базовый объект [xscript.meta](../appendices/block-lua-meta.md) — предоставляет доступ к содержимому контейнера, содержащему [метаинформацию](meta.md) о вызове блока.
- Класс [xscript.cookie](../appendices/block-lua-cookie-methods.md) — предназначен для работы с произвольными куками.
- [Функции таблицы xscript](../appendices/block-lua-other-methods.md) — набор функций различного назначения, предназначенных для использования в XScript.

**Пространство имен xscript.logger**

Содержит функции таблицы [xscript.logger](../appendices/block-lua-logger-methods.md), предназначенные для записи в лог сообщений различного уровня критичности (информация, предупреждение, ошибка).

**Пространство имен xscript.ycookie**

Содержит классы и функции, предназначенные для работы с [Y-куками](http://wiki.yandex-team.ru/Cookies/Y).

- Класс [xscript.ycookie.ys](../appendices/block-lua-ycookie-ys-methods.md).
- Класс [xscript.ycookie.yp](../appendices/block-lua-ycookie-yp-methods.md).
- Класс [xscript.ycookie.gp](../appendices/block-lua-ycookie-gp-methods.md).
- Класс [xscript.ycookie.gpauto](../appendices/block-lua-ycookie-gpauto-methods.md).
- Класс [xscript.ycookie.ygo](../appendices/block-lua-ycookie-ygo-methods.md).
- [Функции таблицы xscript.ycookie](../appendices/block-lua-ycookie-methods.md).


## Вывод результатов в документ {#return}

Существует два способа размещения результатов работы Lua-кода в документе: с помощью оператора `return` и с помощью функции [`print`](../appendices/block-lua-other-methods.md#print).

При использовании оператора return возвращаемое значение интерпретируется как XML-фрагмент, валидируется и размещается дереве формируемого XML-документа.

Когда возвращаемый XML-фрагмент невалиден, в режиме <q>development</q> возвращается ошибка [\<xscript_invoke_failed\>](error-diag-ov.md), а в режиме <q>production</q> инструкция `return` игнорируется, и в лог записывается сообщение об ошибке уровня <q>warning</q>.

Если используется функция print, возвращаемое значение интерпретируется как текстовый узел. При размещении в дереве формируемого XML-документа специальные символы экранируются.

Так в результате выполнения кода

```
<lua>
    <![CDATA[
    print('<output from="print"/>')
    return '<output from="return"/>'
    ]]>
 </lua>
```

В результате обработки данного блока формируется следующий ответ:

```
<lua>
    &lt;output from="print"/&gt;
    <output from="return"/>
</lua>
```

### Узнайте больше {#learn-more}
* [Методы Lua-блока](../appendices/block-lua-state-methods.md)
* [lua](../reference/lua.md)
* [The programming language Lua](http://www.lua.org/)
* [lua-users.org maillist & wiki](http://lua-users.org/)