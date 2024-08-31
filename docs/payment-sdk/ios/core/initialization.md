# Инициализация

## PaymentFactory
Сначала вам нужно проинициализировать [PaymentFactory](../../core/PaymentFactory.md). При создании опционально можно указать тип используемого окружения [PaymentKitMode](../../core/PaymentKitMode.md) и режим логирования [ConsoleLoggingMode](../../core/ConsoleLoggingMode.md), например:
```swift
let factory = PaymentFactory
    .setPaymentKitMode(.debug)
    .setConsoleLoggingMode(.enabled)
    .initialize()
```
После этого можно создать билдер для основного класса [PaymentApi](../../core/PaymentApi.md) вызвав метод [makePaymentApiBuilder](../../core/PaymentFactory.md#makepaymentapibuilderpayermerchantcallbacks).

## PaymentApiBuilder
Билдер позволяет задать множество различных опций, обязательными из них являются:
* [Payer](../../core/Payment_Payer.md) - объект с данными о пользователе
* [Merchant](../../core/Payment_Merchant.md) - объект с данными о сервисе
* [PaymentCallbacks](../../core/PaymentCallbacks.md) - базовые коллбеки для работы с [PaymentApi](../../core/PaymentApi.md)
Пример:
```swift
let payer = Payer(oauthToken: "OAuth token", email: "example@yandex.ru", uid: "999999")
let merchant = Merchant(serviceToken: "trust_service_token_string", localizedName: "localized_name_for_apple_pay")
let paymentApi = factory
    .makePaymentApiBuilder(payer: payer, merchant: merchant, callbacks: SomeCallbacks())
    .build()
```
После вызова build() вы получите основной объект [PaymentApi](../../core/PaymentApi.md)