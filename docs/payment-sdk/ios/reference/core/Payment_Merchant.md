# Payment.Merchant

Данные о сервисе.

``` swift
public struct Merchant: Equatable 
```

## Inheritance

`Equatable`

## Initializers

### `init(serviceToken:localizedName:)`

``` swift
public init(serviceToken: String, localizedName: String) 
```

## Properties

### `serviceToken`

Сервис токен в рамках Траста.

``` swift
public let serviceToken: String
```

### `localizedName`

Локализованное имя для ApplePay. Можно передать пустую строку если не используется ApplePay

``` swift
public let localizedName: String
```
