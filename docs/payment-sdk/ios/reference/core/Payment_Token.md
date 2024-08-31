# Payment.Token

Токен корзины для оплаты.
В случае Яндекс.Оплат это отдельный токен, не совпадает с purchase token Траста.
В остальных случаях эквивалентен purchase token.

``` swift
public struct Token: Equatable 
```

## Inheritance

`Equatable`

## Initializers

### `init(token:)`

``` swift
public init(token: String) 
```

## Properties

### `token`

``` swift
public let token: String
```
