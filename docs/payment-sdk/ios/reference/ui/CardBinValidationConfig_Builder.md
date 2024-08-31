# CardBinValidationConfig.Builder

``` swift
public class Builder 
```

## Initializers

### `init()`

``` swift
public init() 
```

## Methods

### `setBinRangeErrorMessage(_:)`

``` swift
public func setBinRangeErrorMessage(_ message: String) -> Builder 
```

### `addBinRange(from:to:)`

``` swift
public func addBinRange(from: String, to: String) throws -> Builder 
```

### `build()`

``` swift
public func build() -> CardBinValidationConfig 
```
