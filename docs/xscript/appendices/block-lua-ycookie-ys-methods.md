# Методы класса xscript.ycookie.ys

Класс `xscript.cookie.ys` предназначен для работы с сессионной [Y-кукой](http://wiki.yandex-team.ru/Cookies/Y) ys.

#### Список методов: 

- [elements](block-lua-ycookie-ys-methods.md#elements);
- [erase](block-lua-ycookie-ys-methods.md#erase);
- [find](block-lua-ycookie-ys-methods.md#find);
- [insert](block-lua-ycookie-ys-methods.md#insert);
- [new](block-lua-ycookie-ys-methods.md#new);
- [parse](block-lua-ycookie-ys-methods.md#parse);
- [serialize](block-lua-ycookie-ys-methods.md#serialize);
- [setToResponse](block-lua-ycookie-ys-methods.md#set_to_response);

#### `elements()` {#elements}

Возвращает итератор по элементам куки ys. При итерации элемент возвращается в виде таблицы с ключами <q>name</q> и <q>value</q>, содержащими соответствующие значения.

#### `erase(name)` {#erase}

Удаляет из куки ys элемент с именем `name`.

#### `find(name)` {#find}

Производит поиск элемента с именем name (полное соответствие, учитывается регистр). Если элемент найден, возвращается таблица с ключами <q>name</q> и <q>value</q> и соответствующими строковыми значениями.

Если элемент не найден, возвращается nil.

#### `insert(name, value)` {#insert}

Добавляет в куку ys элемент. Входными параметрами являются название элемента и его значение, представленные в виде строк.

Если кука уже содержит элемент с заданным именем, его значение будет изменено на указанное.

В случае успеха возвращает true, в случае неудачи — false.

#### `new()` {#new}

Конструктор. Создает объект, который соответствует куке ys, не содержащей ни одного элемента. 

#### `parse(cookie)` {#parse}

Производит разбор содержимого куки ys и сохраняет полученные данные во внутренней структуре. Кука подается на вход в виде строки.

#### `serialize()` {#serialize}

Преобразует куку ys из внутреннего представления в строковое и возвращает полученное значение.

#### `setToResponse()` {#set_to_response}

Передает куку ys в HTTP-ответ.

**Пример**:

```
<lua>
    local ys = xscript.ycookie.ys.new()
    
    ys:parse('name1.value1#name2.value2#name3.value3')
    ys:insert("name4", "value4")
    
    print("\n"..ys:find("name1").value)
    
    ys:erase("name2")
    
    for elem in ys:elements() do
        print("name: "..elem.name..", value: "..elem.value)
    end
    
    print(ys:serialize().."\n")
</lua>

```

В результате выполнения данного кода в документ будет выведен следующий фрагмент:

```
<lua>
value1
name: name1, value: value1
name: name3, value: value3
name: name4, value: value4
name1.value1#name3.value3#name4.value4
</lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)