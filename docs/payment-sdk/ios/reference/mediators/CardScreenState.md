# CardScreenState

Состояние экрана с вводом данных карт.

``` swift
public enum CardScreenState 
```

## Enumeration Cases

### `idle`

Ожидание.

``` swift
case idle
```

### `loading`

Загрузка.

``` swift
case loading
```

### `successBind`

Успешная привязка.

``` swift
case successBind(card: BoundCard)
```

### `successPay`

Успешная оплата.

``` swift
case successPay(pollingResult: PaymentPollingResult)
```

### `error`

Произошла ошибка.

``` swift
case error(error: PaymentKitError)
```
