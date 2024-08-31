# SimplePaymentMediator

Простой медиатор для процесса оплаты. Управляет состоянием кнопки оплаты, экрана и отображением 3ds, и может сам вызывать передачу кода CVC/CVV когда нужно.

``` swift
public final class SimplePaymentMediator: PaymentCallbacks 
```

## Inheritance

`PaymentCallbacks`

## Properties

### `processing`

Api оплат.

``` swift
public var processing: PaymentProcessingApi?
```

### `paymentCallbacks`

Коллбеки, которые нужно проставить в `PaymentApi` напрямую или через прослойку.

``` swift
public var paymentCallbacks: PaymentCallbacks 
```

## Methods

### `show3ds(url:)`

``` swift
public func show3ds(url: URL) 
```

### `hide3ds()`

``` swift
public func hide3ds() 
```

### `cvvRequested()`

``` swift
public func cvvRequested() 
```

### `newCardDataRequested()`

``` swift
public func newCardDataRequested() 
```
