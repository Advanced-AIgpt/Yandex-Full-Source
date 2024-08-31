# Методы класса xscript.ycookie.yp

Класс `xscript.cookie.yp` предназначен для работы с перманентной [Y-кукой](http://wiki.yandex-team.ru/Cookies/Y) yp. Несмотря на то, что кука yp является перманентной, для каждого ее элемента задается время жизни (в формате [UNIX time stamp](http://ru.wikipedia.org/wiki/Unix_time)).

#### Список методов: 

- [elements](block-lua-ycookie-yp-methods.md#elements);
- [erase](block-lua-ycookie-yp-methods.md#erase);
- [find](block-lua-ycookie-yp-methods.md#find);
- [insert](block-lua-ycookie-yp-methods.md#insert);
- [new](block-lua-ycookie-yp-methods.md#new);
- [parse](block-lua-ycookie-yp-methods.md#parse);
- [serialize](block-lua-ycookie-yp-methods.md#serialize);
- [setToResponse](block-lua-ycookie-yp-methods.md#set_to_response);

#### `elements()` {#elements}

Возвращает итератор по элементам куки yp. При итерации элемент возвращается в виде таблицы с ключами <q>name</q> и <q>value</q>и <q>expire</q>, содержащей соответствующие значения. 

#### `erase(name)` {#erase}

Удаляет из куки yp элемент с именем `name`.

#### `find(name)` {#find}

Производит поиск элемента с именем name (полное соответствие, учитывается регистр). Если элемент найден, возвращается таблица с ключами <q>name</q>, <q>value</q>, <q>expire</q> и соответствующими значениями.

Поля <q>name</q> и <q>value</q> имеют тип string, <q>expire</q> — тип number.

Если элемент не найден, возвращается nil.

#### `insert(element)` {#insert}

Добавляет в куку yp элемент. Входной параметр представляет собой таблицу с ключами <q>name</q>, <q>value</q> и <q>expire</q>.

Поля <q>name</q> и <q>value</q> имеют тип string, <q>expire</q> — тип number.

Если кука уже содержит элемент с заданным именем, значения полей <q>value</q> и <q>expire</q> будут изменены на указанные.В случае успеха возвращает true, в случае неудачи — false.

#### `new()` {#new}

Конструктор. Создает объект, который соответствует куке yp, не содержащей ни одного элемента. 

#### `parse(cookie)` {#parse}

Производит разбор содержимого куки yp и сохраняет полученные данные во внутренней структуре. Кука подается на вход в виде строки.

#### `serialize()` {#serialize}

Преобразует куку yp из внутреннего представления в строковое и возвращает полученное значение.

#### `setToResponse()` {#set_to_response}

Передает куку yp в HTTP-ответ.

**Пример**:

```
<lua>
    local yp = xscript.ycookie.yp.new()
    yp:parse(xscript.request:getCookie('yp'))
    
    local elem = {}
    elem['name'] = 'myelement'
    elem['value'] = 'myvalue'
    elem['expire'] = 1302954782
    yp:insert(elem)
    
    print (yp:serialize())
    
    yp:setToResponse()
</lua>
```

В результате выполнения данного кода в документ будет выведен фрагмент, похожий на приведенный ниже:

```
<lua>1302954782.myelement.myvalue#1299336939.ygu.1</lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)