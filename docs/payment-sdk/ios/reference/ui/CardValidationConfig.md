# CardValidationConfig

Общий конфиг валидации карт.

``` swift
public struct CardValidationConfig 
```

## Initializers

### `init(binConfig:expirationDateConfig:)`

``` swift
public init(binConfig: CardBinValidationConfig = CardBinValidationConfig.default, expirationDateConfig: CardExpirationDateValidationConfig = CardExpirationDateValidationConfig.default) 
```

## Properties

### `binConfig`

``` swift
public let binConfig: CardBinValidationConfig
```

### `expirationDateConfig`

``` swift
public let expirationDateConfig: CardExpirationDateValidationConfig
```
