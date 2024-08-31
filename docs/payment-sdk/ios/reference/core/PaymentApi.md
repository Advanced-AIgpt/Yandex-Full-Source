# PaymentApi

Главный протокол для работы с API - позволяет работать с платежами, привязками банковских карт и ApplePay. Создаётся
через `PaymentApiBuilder`.

``` swift
public protocol PaymentApi 
```

## Default Implementations

### `startPayment(paymentToken:completion:)`

``` swift
func startPayment(paymentToken: Payment.Token, completion: @escaping (PaymentKitResult<PaymentInstance>) -> Void) 
```

### `startPayment(paymentToken:order:completion:)`

``` swift
func startPayment(paymentToken: Payment.Token, order: Order?, completion: @escaping (PaymentKitResult<PaymentInstance>) -> Void) 
```

## Requirements

### startPayment(paymentToken:​order:​isCredit:​completion:​)

Стартовать платёж по заданному токену.

``` swift
func startPayment(paymentToken: Payment.Token, order: Order?, isCredit: Bool, completion: @escaping (PaymentKitResult<PaymentInstance>) -> Void)
```

> 

#### Parameters

  - paymentToken: токен корзины для оплаты.
  - order: информация о заказе.
  - isCredit: будет ли проходить оплата в кредит. Только для работы с Тинькофф кредитами.
  - completion: в коллбек придет либо объект `PaymentInstance`, если старт платежа был успешен или ошибка `PaymentKitError` если нет.

### cancelPayment()

Отменить работу с текущим платежом.

``` swift
func cancelPayment()
```

{% note info %}

Работает аналогично вызову `PaymentInstance.cancel` - не отменяет корзину в самом Трасте, только перестает работать с ней на клиенте.

{% endnote %}

### paymentMethods(completion:​)

``` swift
func paymentMethods(completion: @escaping (PaymentKitResult<[Payment.Method]>) -> Void)
```

  - Получить список методов оплаты, доступный данному пользователю на данном сервисе.
  - **Важно:** для конкретных корзин могут быть доступны особые методы оплаты, если они были заданы
  - при создании и их можно будет получить только в `PaymentInstance.methods`.
  -   - Returns: список методов.

### getInstalledBankApps(completion:​)

Получить доступные банковские приложения на устройстве, поддерживающие Систему Быстрых Платежей.

``` swift
@available(*, deprecated, message: "Use getInstalledBankInfo instead")
    func getInstalledBankApps(completion: @escaping (PaymentKitResult<[Payment.BankAppInfo]>) -> Void)
```

### getInstalledBankInfo(completion:​)

Получить доступные банковские приложения на устройстве, поддерживающие Систему Быстрых Платежей.

``` swift
func getInstalledBankInfo(completion: @escaping (PaymentKitResult<[Payment.BankAppLaunchInfo]>) -> Void)
```

### getAllNspkBankApps(completion:​)

Получить все возможные приложения, поддерживающие Систему Быстрых Платежей.

``` swift
func getAllNspkBankApps(completion: @escaping (PaymentKitResult<[Payment.BankAppLaunchInfo]>) -> Void)
```

### Bind

``` swift
var Bind: BindApi 
```

  - Объект для доступа к `BindApi`.

### ApplePay

``` swift
var ApplePay: ApplePayApi 
```

  - Объект для доступа к `ApplePayApi`.
