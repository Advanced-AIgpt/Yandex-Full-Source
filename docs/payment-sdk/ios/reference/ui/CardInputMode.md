# CardInputMode

Возможные виды вью для ввода данных банковских карт.

``` swift
public enum CardInputMode 
```

## Enumeration Cases

### `bindOnly`

Только привязка карты.

``` swift
case bindOnly
```

### `payAndBind`

Оплата новой картой. В этом случае есть галочка для возможного сохранения карты после успешной оплаты.

``` swift
case payAndBind
```
