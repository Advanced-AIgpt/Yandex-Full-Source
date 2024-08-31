# CardInputMediator

Общий медиатор для работы со вью ввода банковских карт. Содержит в себе базовую логику работы с состояниями вью.

``` swift
public class CardInputMediator: PaymentCallbacks 
```

## Inheritance

`PaymentCallbacks`

## Properties

### `paymentCallbacks`

Коллбеки, которые нужно проставить в `PaymentApi` напрямую или через прослойку.

``` swift
public var paymentCallbacks: PaymentCallbacks 
```

## Methods

### `connectUi(cardInput:webViewDelegate:cardScreenDelegate:actionButton:)`

Состыковать медиатор с UI.

``` swift
public func connectUi(cardInput: CardInput,
                          webViewDelegate: WebView3dsDelegate,
                          cardScreenDelegate: CardScreenDelegate,
                          actionButton: CardActionButton) 
```

#### Parameters

  - cardInput: реализация вью ввода карточных данных.
  - webViewDelegate: делегат для отображения 3ds.
  - cardScreenDelegate: делегат для управления экраном.
  - actionButton: реализация кнопки действия.

### `updateButtonState(state:)`

``` swift
open func updateButtonState(state: CardInputState) -> CardActionButtonState 
```

### `onCardActionButtonClick()`

``` swift
public func onCardActionButtonClick() 
```

### `process()`

Обработать нажатие кнопки.

``` swift
open func process() 
```

### `cancel()`

Отменить запущенный после нажатия кнопки процесс.

``` swift
open func cancel() 
```

### `reset()`

Отсоединиться от всего UI и выполнить сброс.

``` swift
public func reset() 
```

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
