# Модуль xscript.secretkey

**Список функций:**

- getKey;
- getEasyKey;
- check.

#### `getKey`

Возвращает значение [секретного ключа](../concepts/secret-key.md), действительного не менее одних, но не более двух суток. Работает аналогично xslt-функции [get-secret-key](xslt-functions.md#get-secret-key).

#### `getEasyKey`
Возвращает значение [секретного ключа](../concepts/secret-key.md), сгенерированного без использования авторизации по куке `yandexuid`. Ключ действителен не менее одних, но не более двух суток. Работает аналогично xslt-функции [get-easy-secret-key](xslt-functions.md#get-easy-secret-key).
#### `check`
Сравнивает значение действительного секретного ключа с передаваемой в качестве параметра строкой. В случае совпадения возвращает `true`, в противном случае — `false`. Работает аналогично xslt-функции [check-secret-key](xslt-functions.md#check-secret-key).
#### Пример:

```
<x:js>
    var key = xscript.secretkey.getKey()
    var easy_key = xscript.secretkey.getEasyKey()
    xscript.print( key, xscript.secretkey.check(key), easy_key, xscript.secretkey.check(easy_key))
</x:js>

```

Результат:

```xml
<js>
    u51dfa9fc6d0ea8f88256e3a392638d95 true _08d334ca66b790d36eebd481899039c1 true
</js>
```

### Узнайте больше {#learn-more}
* [JavaScript-блок](../concepts/block-js-ov.md)
* [js](../reference/js.md)