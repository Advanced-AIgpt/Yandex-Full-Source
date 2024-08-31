//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[GooglePayApi](index.md)/[makeGooglePayToken](make-google-pay-token.md)

# makeGooglePayToken

[core]\

@MainThread()

abstract fun [makeGooglePayToken](make-google-pay-token.md)(orderDetails: [OrderDetails](../../../com.yandex.payment.sdk.core.data/-order-details/index.md), completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[GooglePayToken](../../../com.yandex.payment.sdk.core.data/-google-pay-token/index.md)>)

Получить от GooglePay токен для заказа. Может быть полезно для привязки платежей и досписаний.

## Parameters

core

| | |
|---|---|
| orderDetails | параметры заказа. |
| completion | коллбек с токеном. |
