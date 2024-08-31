# PaymentKitError

Класс ошибок PaymentSDK

``` swift
public struct PaymentKitError: Error 
```

## Inheritance

`Error`

## Properties

### `kind`

Тип ошибки.

``` swift
public let kind: Kind
```

### `trigger`

Источник возникновения.

``` swift
public let trigger: Trigger
```

### `code`

Код если известен.

``` swift
public let code: Int?
```

### `status`

Статус если известен.

``` swift
public let status: String?
```

### `message`

Сообщение

``` swift
public let message: String
```

### `localizedDescription`

``` swift
public var localizedDescription: String 
```
