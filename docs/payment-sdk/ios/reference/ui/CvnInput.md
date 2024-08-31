# CvnInput

Протокол для работы с логикой ввода CVC/CVV.

``` swift
public protocol CvnInput: UIView 
```

## Inheritance

`UIView`

## Requirements

### setCardPaymentSystem(\_:​)

Задать платёжную систему. В зависимости от платёжной системы может требоваться разное количество знаков в CVC/CVV.

``` swift
func setCardPaymentSystem(_ system: Payment.Card.PaymentSystem)
```

#### Parameters

  - system: платёжная система.

### setOnReadyCallback(\_:​)

Установить коллбек корректности введенного кода. Коллбек будет вызываться при смене состояний.

``` swift
func setOnReadyCallback(_ callback: ((Bool) -> Void)?)
```

{% note info %}

Под корректностью подразумевается правильная длина и наличие только цифр, соответствие же
кода написанному на карте будет проверено только во время оплаты на бэкенде в контуре PCI DSS.

{% endnote %}

#### Parameters

  - callback: коллбек.

### isReady

Корректен ли введённый CVC/CVV код.

``` swift
var isReady: Bool 
```

### provideCvn()

Передать введённый код в `PaymentApi` для продолжения процесса оплаты.

``` swift
func provideCvn()
```

### setPaymentApi(\_:​)

Установить объект `PaymentApi`. В этот объект будет передан код CVC/CVV при вызове `provideCvn`.

``` swift
func setPaymentApi(_ api: PaymentApi?)
```

#### Parameters

  - api: объект PaymentApi

### focusInput()

Установить фокус и поднять клавиатуру на поле ввода. Может использоваться чтобы сразу подготовить вью ко вводу при показе, без дополнительного нажатия пользователем.

``` swift
func focusInput()
```

### reset()

Сбросить инпуты для ввода.

``` swift
func reset()
```
