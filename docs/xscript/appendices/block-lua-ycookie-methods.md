# Функции таблицы xscript.ycookie

#### Список функций:

- [getValue](block-lua-ycookie-methods.md#getvalue)
- [merge](block-lua-ycookie-methods.md#merge)

#### `getValue(name, ys, yp)` {#getvalue}

Производит поиск в куках ys и yp элемента с именем `name` (полное соответствие, учитывается регистр).

Куки ys и yp подаются на вход в виде экземпляров классов [xscript.ycookie.ys](block-lua-ycookie-ys-methods.md) и [xscript.ycookie.yp](block-lua-ycookie-yp-methods.md) соответственно, name — в виде строки. Элементы в куке ys считаются более приоритетными.

Значение найденного элемента возвращается в виде строки. Если элемент с именем `name` не найден, возвращается nil.

#### `merge(ys, yp)` {#merge}

Объединяет элементы кук ys и yp. Результат возвращается в виде таблицы, ключи которой совпадают с названиями элементов, а значения - со значениями элементов.

В процессе объединения удаляются дубликаты элементов с одинаковым именем. При этом элементы куки ys считаются более приоритетными. Кроме того, удаляются элементы куки yp время жизни которых меньше текущего.

Любой из входных параметров может быть передан либо в виде строки либо в виде экземпляра соответствующего класса: [xscript.ycookie.ys](block-lua-ycookie-ys-methods.md) или [xscript.ycookie.yp](block-lua-ycookie-yp-methods.md)

**Пример**:

```
<lua>
    local yp = xscript.ycookie.yp.new()
    yp:parse("1292954782.name2.value2#1295944782.name1.value1#1296944782.name3.value3")
 
    local ys = xscript.ycookie.ys.new()
    ys:parse("name2.value22");
 
    local val = xscript.ycookie.getValue("name2", ys, yp)
    
    local merged = xscript.ycookie.merge(ys, yp);
    
    print(val..':has to be equal to:'.. merged.name2)
</lua>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)
* [Методы класса xscript.ycookie.ys](../appendices/block-lua-ycookie-ys-methods.md)
* [Методы класса xscript.ycookie.yp](../appendices/block-lua-ycookie-yp-methods.md)