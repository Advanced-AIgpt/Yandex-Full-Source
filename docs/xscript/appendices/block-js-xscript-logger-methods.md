# Функции пространства имен xscript.logger

Функции для вывода сообщений в лог XScript.

#### Список функций:

- [info](block-js-xscript-logger-methods.md#info)
- [error](block-js-xscript-logger-methods.md#error)
- [warn](block-js-xscript-logger-methods.md#warn)

Функции принимают на вход один или несколько параметров. При вызове функции параметры преобразуются в строки, последовательно конкатенируются, полученное значение записывается в лог с указанием уровня сообщения.

#### `info(str1 [, str2, ..., strN])` {#info}

Записывает в лог XScript-а сообщение уровня INFO, содержащее результат конкатенации значений аргументов.

#### `error(str1 [, str2, ..., strN])` {#error}

Записывает в лог XScript-а сообщение уровня ERROR, содержащее результат конкатенации значений аргументов.

#### `warn(str1 [, str2, ..., strN])` {#warn}

Записывает в лог XScript-а сообщение уровня WARN, содержащее результат конкатенации значений аргументов.

**Пример использования**:

```
<?xml version="1.0" ?>
<page xmlns:x="http://www.yandex.ru/xscript">
  <x:js>
    xscript.logger.info('JavaScript information message');
    xscript.logger.info('TEST LOG INFO: ', 'PARAM1');
    xscript.logger.warn('TEST LOG WARN: ', 'PARAM1', ' PARAM2');
    xscript.logger.error('TEST LOG ERROR: ', 'PARAM1 ', 'PARAM2', 'PARAM-PARAM');
  </x:js>
</page>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)
