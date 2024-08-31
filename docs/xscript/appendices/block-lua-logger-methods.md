# Функции таблицы xscript.logger

#### Список функций:

- [info](block-lua-logger-methods.md#info)
- [error](block-lua-logger-methods.md#error)
- [warn](block-lua-logger-methods.md#warn)

#### `info(str)` {#info}

Записывает в лог XScript-а строку `str` с уровнем логирования INFO.

#### `error(str)` {#error}

Записывает в лог XScript-а строку `str` с уровнем логирования ERROR.

#### `warn(str)` {#warn}

Записывает в лог XScript-а строку `str` с уровнем логирования WARN.

**Пример использования**:

```
<?xml version="1.0" ?>
<?xml-stylesheet type="text/xsl" href="object.xsl"?>
<page xmlns:x="http://www.yandex.ru/xscript" xmlns:xi="http://www.w3.org/2001/XInclude">
    <lua>
        xscript.logger.error('ERROR')
        xscript.logger.warn('WARN')
        xscript.logger.info('INFO')      
    </lua>
</page>
```

### Узнайте больше {#learn-more}
* [Lua-блок](../concepts/block-lua-ov.md)
* [lua](../reference/lua.md)