# BindProcessingApi

Протокол для привязки карт.

``` swift
public protocol BindProcessingApi 
```

## Requirements

### bindCard(completion:​)

Привязать карту.

``` swift
func bindCard(completion: @escaping (PaymentKitResult<BoundCard>) -> Void)
```

#### Parameters

  - completion: комплишен с результатом.

### cancel()

Отменить привязку.

``` swift
func cancel()
```
