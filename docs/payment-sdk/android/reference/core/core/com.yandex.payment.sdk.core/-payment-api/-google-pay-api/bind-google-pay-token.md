//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[GooglePayApi](index.md)/[bindGooglePayToken](bind-google-pay-token.md)

# bindGooglePayToken

[core]\

@MainThread()

abstract fun [bindGooglePayToken](bind-google-pay-token.md)(googlePayToken: [GooglePayToken](../../../com.yandex.payment.sdk.core.data/-google-pay-token/index.md), orderTag: String, completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[GooglePayTrustMethod](../../../com.yandex.payment.sdk.core.data/-google-pay-trust-method/index.md)>)

Привязать токен GooglePay в Trust.

## Parameters

core

| | |
|---|---|
| googlePayToken | токен GooglePay. |
| orderTag | тэг заказа |
| completion | коллбек с результатом. |
