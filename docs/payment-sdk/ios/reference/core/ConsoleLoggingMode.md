# ConsoleLoggingMode

Режим логирования PaymentSDK.

``` swift
public enum ConsoleLoggingMode: String 
```

## Inheritance

`String`

## Enumeration Cases

### `automatic`

Логировать только в тестовом окружении.

``` swift
case automatic
```

### `disabled`

Не логировать.

``` swift
case disabled
```

### `enabled`

Логировать всегда. **Важно:​** не использовать в production\!

``` swift
case enabled
```
