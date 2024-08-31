# Banner-блок

Banner-блок предназначен для генерации хэша параметров страницы на основании алгоритма [FNV32](http://www.isthe.com/chongo/tech/comp/fnv/). Полученный хэш затем передается методу CORBA-объекта и используется Баннерной системой.

**Пример Banner-блока**:

```
<banner-block>
    <method>set_state_hash</method>
    <param type="String">page-tag</param>
    <param type="String">mapID</param>
    <param type="String">mapX</param>
    <param type="String">mapY</param>
    <param type="String">scale</param>
    <param type="String">slices</param>
</banner-block>
```

В приведенном примере выполняется получение хэша параметров страницы, используемого Баннерной системой, который сохраняется в переменную контейнера [State](state-ov.md).

### Узнайте больше {#learn-more}
* [Методы Banner-блока](../appendices/block-banner-methods.md)
* [banner-block](../reference/banner-block.md)