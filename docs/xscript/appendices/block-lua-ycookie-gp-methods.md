# Методы класса xscript.ycookie.gp

Класс `xscript.cookie.gp` предназначен для работы с [(cуб)кукой gp](http://wiki.yandex-team.ru/morda/tune/region#logikaraboty), включаемой в [Y-куки](http://wiki.yandex-team.ru/Cookies/Y) ys и yp.

#### Список методов: 

- [elements](block-lua-ycookie-gp-methods.md#elements);
- [erase](block-lua-ycookie-gp-methods.md#erase);
- [find](block-lua-ycookie-gp-methods.md#find);
- [insert](block-lua-ycookie-gp-methods.md#insert);
- [new](block-lua-ycookie-gp-methods.md#new);
- [parse](block-lua-ycookie-gp-methods.md#parse);
- [serialize](block-lua-ycookie-gp-methods.md#serialize);

#### `elements()` {#elements}

Возвращает итератор по элементам куки gp. При итерации элемент возвращается в виде таблицы с ключами с ключами <q>id</q>, <q>latitude</q>, <q>longitude</q>, <q>region</q>, <q>default</q>, <q>name</q>, содержащей соответствующие значения. 

#### `erase(name)` {#erase}

Удаляет из куки gp элемент с именем `name`.

#### `find(name)` {#find}

Производит поиск элемента с именем name (полное соответствие, учитывается регистр). Если элемент найден, возвращается таблица с ключами <q>id</q>, <q>latitude</q>, <q>longitude</q>, <q>region</q>, <q>default</q>, <q>name</q> и соответствующими значениями.

Поля <q>id</q>, <q>latitude</q>, <q>longitude</q>, <q>region</q> имеют тип number, <q>default</q> — тип number, <q>name</q> — тип string.

Если элемент не найден, возвращается nil.

#### `insert(element)` {#insert}

Добавляет в куку gp элемент. Входной параметр представляет собой таблицу с ключами <q>id</q>, <q>latitude</q>, <q>longitude</q>, <q>region</q>, <q>default</q>, <q>name</q>.

Поля <q>id</q>, <q>latitude</q>, <q>longitude</q>, <q>region</q> имеют тип number, <q>default</q> — тип boolean, <q>name</q> — тип string.

Если кука уже содержит элемент с заданным id, значения полей таблицы будут изменены на указанные.В случае успеха возвращает true, в случае неудачи — false.

#### `new()` {#new}

Конструктор. Создает объект, который соответствует куке gp, не содержащей ни одного элемента. 

#### `parse(cookie)` {#parse}

Производит разбор содержимого куки gp и сохраняет полученные данные во внутренней структуре. Кука подается на вход в виде строки.

#### `serialize()` {#serialize}

Преобразует куку gp из внутреннего представления в строковое и возвращает полученное значение.

**Пример**:

```
<lua>

    local yp = xscript.ycookie.yp.new()
    yp:parse(xscript.request:getCookie('yp'));
 
    -- создаём субкуку gp
    local gp = xscript.ycookie.gp.new()
    -- создаем точку
    local point = {};
    point['id'] = 1
    point['latitude'] = 135.4
    point['longitude'] = 34.6
    point['region'] = 213
    point['default'] = false
    point['name'] = 'Москва'
    -- помещаем точку в субкуку gp
    gp:insert(point);
     
    -- субкуку gp вкладываем в куку yp
    -- как элемент с именем "mygp"
    local el = {};
    el['name'] = 'mygp'
    el['value'] = gp:serialize()
    el['expire'] = 1302954782
    yp:insert(el)
    
    local gp_elem = yp:find('mygp') -- ищем в куке yp элемент с именем "mygp"
    gp:parse(gp_elem.value) -- перезаполняем объект gp данными из найденного элемента
     
    for point in gp:elements() do -- итерируемся по точкам в контейнере gp
        latitude = point.latitude
        longitude = point.longitude
        region = point.region
        name = point.name
        print(region..':'..name) -- выводим номер региона и название
    end
        
</lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)