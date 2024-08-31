# Order.Builder

``` swift
public class Builder 
```

## Initializers

### `init()`

``` swift
public init() 
```

## Methods

### `setOrderTag(_:)`

``` swift
public func setOrderTag(_ orderTag: Tag) -> Builder 
```

### `setCurrencyAmount(_:)`

``` swift
public func setCurrencyAmount(_ currencyAmount: CurrencyAmount) -> Builder 
```

### `setPaymentSheet(_:)`

``` swift
public func setPaymentSheet(_ paymentSheet: PaymentSheet) -> Builder 
```

### `setIsFinalAmount(_:)`

``` swift
public func setIsFinalAmount(_ isFinalAmount: Bool) -> Builder 
```

### `build()`

``` swift
public func build() -> Order 
```
