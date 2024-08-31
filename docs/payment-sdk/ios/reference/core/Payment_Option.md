# Payment.Option

``` swift
public struct Option: Equatable 
```

## Inheritance

`Equatable`

## Initializers

### `init(id:account:system:familyInfo:partnerInfo:)`

``` swift
public init(id: String, account: String, system: String, familyInfo: FamilyInfo?, partnerInfo: PartnerInfo?) 
```

## Properties

### `id`

``` swift
public let id: String
```

### `account`

``` swift
public let account: String
```

### `system`

``` swift
public let system: String
```

### `familyInfo`

``` swift
public let familyInfo: FamilyInfo?
```

### `partnerInfo`

``` swift
public let partnerInfo: PartnerInfo?
```

## Methods

### `newCard(account:system:)`

``` swift
public static func newCard(account: String, system: String) -> Option 
```

### `applePay()`

``` swift
public static func applePay() -> Option 
```

### `sbp()`

``` swift
public static func sbp() -> Option 
```

### `cash()`

``` swift
public static func cash() -> Option 
```

### `tinkoffCredit()`

``` swift
public static func tinkoffCredit() -> Option 
```
