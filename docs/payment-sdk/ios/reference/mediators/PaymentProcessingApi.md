# PaymentProcessingApi

Протокол для оплаты.

``` swift
public protocol PaymentProcessingApi 
```

## Requirements

### pay(completion:​)

Оплатить.

``` swift
func pay(completion: @escaping (PaymentKitResult<PaymentPollingResult>) -> Void)
```

#### Parameters

  - completion: комплишен с результатом.

### cancel()

Отменить процесс оплаты.

``` swift
func cancel()
```
