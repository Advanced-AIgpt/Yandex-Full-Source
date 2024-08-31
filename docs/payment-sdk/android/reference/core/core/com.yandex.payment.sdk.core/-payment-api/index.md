//[core](../../../index.md)/[com.yandex.payment.sdk.core](../index.md)/[PaymentApi](index.md)

# PaymentApi

[core]\
interface [PaymentApi](index.md)

Главный интерфейс для работы с API - позволяет работать с платежами, привязками банковских карт и GooglePay. Создается через [Builder](-builder/index.md).

## Types

| Name | Summary |
|---|---|
| [BindApi](-bind-api/index.md) | [core]<br>interface [BindApi](-bind-api/index.md)<br>Интерфейс для работы с банковскими картами. |
| [Builder](-builder/index.md) | [core]<br>class [Builder](-builder/index.md)<br>Билдер для создания объекта [PaymentApi](index.md). |
| [GooglePayApi](-google-pay-api/index.md) | [core]<br>interface [GooglePayApi](-google-pay-api/index.md)<br>Интерфейс для работы с GooglePay. |
| [Payment](-payment/index.md) | [core]<br>interface [Payment](-payment/index.md)<br>Интерфейс для работы с оплатой. |

## Functions

| Name | Summary |
|---|---|
| [cancelPayment](cancel-payment.md) | [core]<br>@MainThread()<br>abstract fun [cancelPayment](cancel-payment.md)()<br>Отменить работу с текущим платежом. |
| [getAllNspkBankApps](get-all-nspk-bank-apps.md) | [core]<br>@MainThread()<br>abstract fun [getAllNspkBankApps](get-all-nspk-bank-apps.md)(completion: [PaymentCompletion](../index.md#152061939%2FClasslikes%2F-2113150450)<List<[BankAppInfo.ServerInfo](../../com.yandex.payment.sdk.core.data/-bank-app-info/-server-info/index.md)>>)<br>Получить все возможные приложения, поддерживающие Систему Быстрых Платежей. |
| [getInstalledBankApps](get-installed-bank-apps.md) | [core]<br>@MainThread()<br>~~abstract~~ ~~fun~~ [~~getInstalledBankApps~~](get-installed-bank-apps.md)~~(~~~~completion~~~~:~~ [PaymentCompletion](../index.md#152061939%2FClasslikes%2F-2113150450)<List<ResolveInfo>>~~)~~<br>Получить доступные банковские приложения на устройстве, поддерживающие Систему Быстрых Платежей. |
| [getInstalledBankInfo](get-installed-bank-info.md) | [core]<br>@MainThread()<br>abstract fun [getInstalledBankInfo](get-installed-bank-info.md)(completion: [PaymentCompletion](../index.md#152061939%2FClasslikes%2F-2113150450)<List<[BankAppInfo.DeviceInfo](../../com.yandex.payment.sdk.core.data/-bank-app-info/-device-info/index.md)>>)<br>Получить доступные банковские приложения на устройстве, поддерживающие Систему Быстрых Платежей. |
| [paymentMethods](payment-methods.md) | [core]<br>@WorkerThread()<br>abstract fun [paymentMethods](payment-methods.md)(): [Result](../../com.yandex.payment.sdk.core.data/-result/index.md)<List<[PaymentMethod](../../com.yandex.payment.sdk.core.data/-payment-method/index.md)>><br>Получить список методов оплаты, доступный данному пользователю на данном сервисе. |
| [startPayment](start-payment.md) | [core]<br>@MainThread()<br>abstract fun [startPayment](start-payment.md)(paymentToken: [PaymentToken](../../com.yandex.payment.sdk.core.data/-payment-token/index.md), orderInfo: [OrderInfo](../../com.yandex.payment.sdk.core.data/-order-info/index.md)?, isCredit: Boolean = false, completion: [PaymentCompletion](../index.md#152061939%2FClasslikes%2F-2113150450)<[PaymentApi.Payment](-payment/index.md)>)<br>Стартовать платёж по заданному токену. |

## Properties

| Name | Summary |
|---|---|
| [Bind](-bind.md) | [core]<br>abstract val [Bind](-bind.md): [PaymentApi.BindApi](-bind-api/index.md)<br>Объект для доступа к [BindApi](-bind-api/index.md). |
| [GPay](-g-pay.md) | [core]<br>abstract val [GPay](-g-pay.md): [PaymentApi.GooglePayApi](-google-pay-api/index.md)<br>Объект для доступа к [GooglePayApi](-google-pay-api/index.md). |
