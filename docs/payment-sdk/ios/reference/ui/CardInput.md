# CardInput

Протокол для работы с логикой ввода данных банковских карт.

``` swift
public protocol CardInput: UIView 
```

## Inheritance

`UIView`

## Requirements

### setOnStateChangeCallback(\_:​)

Установить коллбек для отслеживания смены состояний.

``` swift
func setOnStateChangeCallback(_ callback: ((CardInputState) -> Void)?)
```

#### Parameters

  - callback: коллбек.

### setMaskedCardNumberCallback(\_:​)

Установить листенер для получения маскированного номера карты. При введении корректного номера вызывается с маскированным номером, при некоррректном вызывается с `nil`

``` swift
func setMaskedCardNumberCallback(_ callback: ((String?) -> Void)?)
```

#### Parameters

  - callback: коллбек.

### setCardPaymentSystemCallback(\_:​)

Установить листенер для определения платёжной системы. Срабатывает как только логика смогла определить платёжную систему по введённой части номера.

``` swift
func setCardPaymentSystemCallback(_ callback: ((Payment.Card.PaymentSystem) -> Void)?)
```

#### Parameters

  - callback: коллбек.

### setSaveCardOnPayment(\_:​)

Нужно ли сохранять карту после успешной оплаты. Имеет значение только для режима \`CardInputMode.payAndBind.

``` swift
func setSaveCardOnPayment(_ save: Bool)
```

#### Parameters

  - save: `true` если нужно сохранить.

### proceedToCardDetails()

Перейти в состояние с отображением даты и CVC/CVV.

``` swift
func proceedToCardDetails()
```

### provideCardData()

Передать карточные данные в `PaymentApi`.

``` swift
func provideCardData()
```

### setPaymentApi(\_:​)

Установить объект `PaymentApi`, в него будут переданы данные после вызова `provideCardData`.

``` swift
func setPaymentApi(_ api: PaymentApi)
```

#### Parameters

  - api: валидный PaymentApi.

### focusInput()

Установить фокус и поднять клавиатуру на поле ввода. Может использоваться чтобы сразу подготовить вью ко вводу при показе, без дополнительного нажатия пользователем.

``` swift
func focusInput()
```

### reset()

Сбросить все поля ввода.

``` swift
func reset()
```

### setOnHeightChangedCallback(\_:​)

Коллбек что высота вью изменилась, может быть полезен если используются Frames для UI.

``` swift
func setOnHeightChangedCallback(_ callback: (() -> Void)?)
```

#### Parameters

  - callback: коллбек.
