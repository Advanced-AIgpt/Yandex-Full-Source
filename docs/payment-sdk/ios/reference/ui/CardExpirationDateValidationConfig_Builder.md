# CardExpirationDateValidationConfig.Builder

``` swift
public class Builder 
```

## Initializers

### `init()`

``` swift
public init() 
```

## Methods

### `setMinExpirationDate(year:month:errorMessage:)`

``` swift
public func setMinExpirationDate(year: Int32, month: Int32, errorMessage: String?) throws -> Builder 
```

### `setAllowEndlessCards(allow:)`

``` swift
public func setAllowEndlessCards(allow: Bool) 
```

### `build()`

``` swift
public func build() -> CardExpirationDateValidationConfig 
```
