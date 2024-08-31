//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[GooglePayApi](index.md)

# GooglePayApi

[core]\
interface [GooglePayApi](index.md)

Интерфейс для работы с GooglePay. Содержит вспомогательные методы, не работающие с самими платежами в Трасте.

## Functions

| Name | Summary |
|---|---|
| [bindGooglePayToken](bind-google-pay-token.md) | [core]<br>@MainThread()<br>abstract fun [bindGooglePayToken](bind-google-pay-token.md)(googlePayToken: [GooglePayToken](../../../com.yandex.payment.sdk.core.data/-google-pay-token/index.md), orderTag: String, completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[GooglePayTrustMethod](../../../com.yandex.payment.sdk.core.data/-google-pay-trust-method/index.md)>)<br>Привязать токен GooglePay в Trust. |
| [isGooglePayAvailable](is-google-pay-available.md) | [core]<br>@MainThread()<br>abstract fun [isGooglePayAvailable](is-google-pay-available.md)(completion: (Boolean) -> Unit)<br>Проверить, доступен ли на устройстве GooglePay для оплаты. |
| [makeGooglePayToken](make-google-pay-token.md) | [core]<br>@MainThread()<br>abstract fun [makeGooglePayToken](make-google-pay-token.md)(orderDetails: [OrderDetails](../../../com.yandex.payment.sdk.core.data/-order-details/index.md), completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[GooglePayToken](../../../com.yandex.payment.sdk.core.data/-google-pay-token/index.md)>)<br>Получить от GooglePay токен для заказа. |
