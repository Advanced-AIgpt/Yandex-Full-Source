# Order

Параметры заказа для ApplePay.

``` swift
public struct Order 
```

## Properties

### `tag`

Тэг заказа.

``` swift
public let tag: Tag?
```

### `currencyAmount`

Сумма заказа.

``` swift
public let currencyAmount: CurrencyAmount?
```

### `paymentSheet`

Список позиций для отображения в контроллере ApplePay.

``` swift
public let paymentSheet: PaymentSheet?
```

### `isFinalAmount`

Известна ли финальная цена.

``` swift
public let isFinalAmount: Bool
```
