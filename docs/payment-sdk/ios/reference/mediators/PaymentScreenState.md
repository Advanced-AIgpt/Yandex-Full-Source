# PaymentScreenState

Возможные состояния экрана оплаты.

``` swift
public enum PaymentScreenState 
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

### `success`

Успешная оплата.

``` swift
case success(result: PaymentPollingResult)
```

### `error`

Произошла ошибка.

``` swift
case error(error: PaymentKitError)
```
