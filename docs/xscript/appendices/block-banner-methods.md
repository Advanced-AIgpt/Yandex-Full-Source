# Методы Banner-блока

**Список методов Banner-блока**:
- [set_state_hash](block-banner-methods.md#set_state_hash);
- [set_state_page_tag](block-banner-methods.md#set_state_page_tag).

#### `set_state_hash` (`setStateHash`) {#set_state_hash}

Принимает набор префиксов имен переменных из [State](../concepts/state-ov.md), на основании которых согласно алгоритму FNV32 строит хэш-значение, используемое Баннерной системой.

**Входные параметры**: `<result-state-name> <prefix-1> [<prefix-2>...`
- `result-state-name` - имя переменной в State, в которую записывается полученное хэш-значение;
- `prefix-1...prefix-n` - список префиксов имен переменных, на основании которых рассчитывается хэш.

**Пример использования**:
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

#### `set_state_page_tag` (`setStatePageTag`) {#set_state_page_tag}

Принимает список параметров и на их основании согласно алгоритму FNV32 строит хэш-значение, используемое Баннерной системой.

**Входные параметры**: `<result-state-name> <value-1> [<value-2>... <value-n>]`

- `result-state-name` - имя переменной в State, в которую записывается полученное хэш-значение;
- `value-1...value-n` - список параметров, на основании которых рассчитывается хэш.

**Пример использования**:
```
<banner-block>
        <method>set_state_page_tag</method>
        <param type="String">page-tag</param>
        <param type="StateArg">afisha-rubric</param>
        <param type="QueryArg">date</param>
        <param type="QueryArg">city</param>
</banner-block>
```

### Узнайте больше {#learn-more}
* [Banner-блок](../concepts/block-banner-ov.md)
* [banner-block](../reference/banner-block.md)
