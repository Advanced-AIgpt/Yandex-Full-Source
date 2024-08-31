# Метаинформация о вызове блока

В XScript реализована возможность получить метаинформацию о вызове блока в виде XML-дерева.

Метаинформация представлена в XScript как контейнер, состоящий из пар "ключ-значение" с уникальными ключами. Ключи _elapsed-time_ ( время работы блока в миллисекундах), _expire-time_ (время в unixtime, до которого валиден кэш) и _last-modified_ (время последнего изменения информации в кэше) зарезервированы. Контейнер существует только во время выполнения блока.

Для работы с метаинформацией используется тег [\<meta\>](../reference/meta-tag.md), в теле которого записывается [Lua-код](meta.md#lua). В результате обработки _\<meta\>_ формируется [XML-выдача](meta.md#xml), к которой могут быть применены [XPointer](../appendices/xpointer.md)- и [XPath](../reference/xpath.md)-выражения. По умолчанию применяется XPointer-выражение «/..», то есть результат обработки секции - пустой.

В теге _\<meta\>_ обязательно должно быть указано пространство имен xscript:

```
<x:meta>
```

В случае отсутствия в блоке секции _\<meta\>_, контейнер не заполняется.

По умолчанию корневым элементом ответа является тег _\<meta\>_. Однако его можно переопределить, указав в теге _\<meta\>_ атрибут _name_:

```
<x:meta name="g:page">
```

В случае возникновения ошибок при обработке секции \<meta\> возвращается ошибка `<meta_invoke_failed>`. При этом блок выполняется как обычно.

При кэшировании результатов работы блока кэшируется то содержимое контейнера с метаинформацией, которое сформировано во время работы блока.


## Последовательность выполнения операций при вызове блока с секцией \<meta\> {#steps}

Предположим, вызван следующий Local-блок с секцией _\<meta\>_:

```
<x:local proxy="yes">
    <root>
        <mist method="dumpState"/>
        <lua>xscript.meta:setString("key0", "value0")</lua>
    </root>
    <x:meta xpointer="/*">
        <xpath expr="/meta/key0" result="key"/>
        <x:lua>
            xscript.meta:setString("key1", "value1")
            xscript.meta:setString("key2", "value2")
            local t = {a=1, b=2}
            xscript.meta:setTable("key3", t)
            local p = {3, 4}
            xscript.meta:setTable("key4", p)
        </x:lua>
    </x:meta>
</x:local>
```

Из данного примера видно, что к содержимому контейнера c метаинформацией можно получить доступ из Lua-блока, вызванного из дочернего скрипта [Local-блока](block-local-ov.md), так и из секции _\<meta\>_.

Последовательность выполнения операций при обработке этого кода будет следующей:

1. Выполнение блока. На этом шаге заполняется контейнер с метаинформацией. Поля _elapsed-time_, _expire-time_ и _last-modified_ XScript заполняет автоматически.
1. Наложение [перблочного преобразования](per-block-transformation-ov.md) на результат работы блока.
1. Выполнение Lua-кода, записанного в _\<meta\>_. С помощью этого кода можно изменить метаинформацию, полученную в результате работы блока, в частности, управлять кэшированием.
1. Применение XPath-выражения (если оно определено) к результату работы блока.
1. XML-сериализация контейнера с метаинформацией:{#xml}
    
    ```
    <x:meta>
    <param type="LongLong" name="elapsed-time">5</param>
    <!--- Сериализация переменной простого типа с именем "key0" и значением "value0" --->
    <param type="String" name="key0">value0</param>
    <param type="String" name="key1">value1</param>
    <param type="String" name="key2">value2</param>
    <!--- Сериализация переменной типа Map с именем "key3" и значениями "a=1" и "b=2" --->
    <param type="Map" name="key3">
    <param type="Double" name="a">1</param>
    <param type="Double" name="b">2</param>
    </param>
    <!--- Сериализация переменной типа Array с именем "key4" и значениями "3" и "4" --->
    <param type="Array" name="key4">
    <param type="Double">3</param>
    <param type="Double">4</param>
    </param>
    ....
    </x:meta>
    ```
    
1. Применение XPath-выражения (если оно определено) к результату обработки _\<meta\>_.
1. Применение XPointer к результатам работы блока.
1. Применение XPointer к результату обработки _\<meta\>_.
1. Склеивание XML-фрагментов, полученных в результате работы блока и обработки _\<meta\>_. В результате формируется node list, в котором результаты обработки _\<meta\>_ следуют непосредственно за результатами работы блока:
    
    ```
    <root>
    <state_dump/>
    <lua/>
    </root>
    <x:meta>
    <param type="LongLong" name="elapsed-time">5</param>
    <param type="String" name="key0">value0</param>
    <param type="String" name="key1">value1</param>
    <param type="String" name="key2">value2</param>
    <param type="Map" name="key3">
    <param type="Double" name="a">1</param>
    <param type="Double" name="b">2</param>
    </param>
    <param type="Array" name="key4">
    <param type="Double">3</param>
    <param type="Double">4</param>
    </param>
    </x:meta>
    ```
    
    Кроме того, в контейнер State будет записана переменная с именем “key” и значением “value0”.


## Работа с метаинформацией из Lua {#lua}

Для работы с метаинформацией о вызове блока в Lua добавлен объект _xscript.meta_.

Объект xscript.meta доступен как из управляющего Lua-кода секции _\<meta\>_, так и из Lua-блоков. При использовании в Lua-блоке объект xscript.meta позволяет работать с метаинформацией блока, родительского по отношению к Lua-блоку, а если родительский блок отсутствует, объект будет пустым. Для доступа к контейнеру c метаинформацией Lua-блока необходимо использовать объект _xscript.selfmeta_. Для него доступны те же функции, что и для xscript.meta.

В Lua-коде секции \<meta\> объекты xscript.meta и xscript.selfmeta ссылаются на один и тот же контейнер c метаинформацией для основного блока.

```
<x:local proxy="yes">
    <root>
        <!-- Добавление в контейнер с метаинформацией Local-блока переменную key0 со значением "value0" -->
        <lua>xscript.meta:setString("key0", "value0")</lua> 
        <lua>
            <!-- Добавление в контейнер с метаинформацией Lua-блока переменную key1 со значением "value1" -->
            xscript.selfmeta:setString("key1", "value1") 
            <x:meta xpointer="/*"/>
        </lua>
    </root>
    <x:meta xpointer="/*">
        <x:lua>
            <!-- Добавление в контейнер с метаинформацией Local-блока переменную key2 со значением "value2" -->
            xscript.meta:setString("key2", "value2")
            <!-- Добавление в контейнер с метаинформацией Local-блока переменную key3 со значением "value3" -->
            xscript.selfmeta:setString("key3", "value3")
        </x:lua>
    </x:meta>
</x:local>
```


## Метаинформация HTTP-блока {#http}

[HTTP-блок](block-http-ov.md) добавляет в контейнер с метаинформацией полученные HTTP-заголовки.

В качестве ключа используется указанное в верхнем регистре имя заголовка с префиксом "HTTP_". Если несколько заголовков имеют одинаковое имя (например, _Set-Cookie_), в контейнер добавляется переменная типа Array со значениями заголовков.

**Пример**:

```
<x:meta>
    <param type="LongLong" name="elapsed-time">16</param>
    <param type="String" name="HTTP_CONNECTION">close</param>
    <param type="String" name="HTTP_CONTENT-TYPE">text/xml; charset=windows-1251</param>
    <param type="String" name="HTTP_DATE">Fri, 18 Jun 2010 08:53:08 GMT</param>
    <param type="String" name="HTTP_EXPIRES">Fri, 18 Jun 2010 08:58:08 GMT</param>
    <param type="String" name="HTTP_SERVER">lighttpd/1.4.19</param>
    <param type="Array" name="HTTP_SET-COOKIE">
        <param type="String">yandexuid=339345634560131275556851188; domain=.yandex.ru; path=/; expires=Tue, 19 Jan 2038 03:14:07 GMT</param>
        <param type="String">yandexuid2=343678563575757566855551188; domain=.yandex.ru; path=/; expires=Tue, 19 Jan 2038 03:14:07 GMT</param>
    </param>
    <param type="String" name="HTTP_TRANSFER-ENCODING">chunked</param>
</x:meta>
```


## Обработка ошибок {#error}

В случае ошибки выполнения блока обработка секции _\<meta\>_ не производится.

В случае возникновения ошибок при обработке секции _\<meta\>_ возвращается ошибка `<meta_invoke_failed>`. При этом блок выполняется как обычно.

### Узнайте больше {#learn-more}
* [Управление кэшированием с помощью \<meta\>](../concepts/meta-cache.md)
* [meta](../reference/meta-tag.md)
* [Методы объекта xscript.meta](../appendices/block-lua-meta.md)
* [Обработка блока с секцией \<meta\>](../concepts/block-handling-with-meta.md)