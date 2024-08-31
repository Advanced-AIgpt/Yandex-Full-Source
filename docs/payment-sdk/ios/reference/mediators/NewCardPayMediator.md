# NewCardPayMediator

Медиатор для оплаты новой картой. Дополнительно проверяет, что пользовательский email не пуст -
это может быть полезно для анонимных платежей.

``` swift
public final class NewCardPayMediator: CardInputMediator 
```

{% note info %}

Проверяется только non nil, валидность приложение должно контролировать само.

{% endnote %}

## Inheritance

[`CardInputMediator`](/CardInputMediator)

## Initializers

### `init()`

``` swift
public override init() 
```

## Properties

### `processing`

Api для оплат.

``` swift
public var processing: PaymentProcessingApi?
```

## Methods

### `onUserEmailChanged(email:)`

Пользователь изменил email.

``` swift
public func onUserEmailChanged(email: String?) 
```

#### Parameters

  - email: новый email или `nil`.

### `updateButtonState(state:)`

``` swift
public override func updateButtonState(state: CardInputState) -> CardActionButtonState 
```

### `process()`

``` swift
public override func process() 
```

### `cancel()`

``` swift
public override func cancel() 
```
