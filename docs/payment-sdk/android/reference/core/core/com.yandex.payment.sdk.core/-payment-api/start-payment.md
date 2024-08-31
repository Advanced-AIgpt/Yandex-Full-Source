//[core](../../../index.md)/[com.yandex.payment.sdk.core](../index.md)/[PaymentApi](index.md)/[startPayment](start-payment.md)

# startPayment

[core]\

@MainThread()

abstract fun [startPayment](start-payment.md)(paymentToken: [PaymentToken](../../com.yandex.payment.sdk.core.data/-payment-token/index.md), orderInfo: [OrderInfo](../../com.yandex.payment.sdk.core.data/-order-info/index.md)?, isCredit: Boolean = false, completion: [PaymentCompletion](../index.md#152061939%2FClasslikes%2F-2113150450)<[PaymentApi.Payment](-payment/index.md)>)

Стартовать платёж по заданному токену.

## See also

core

| | |
|---|---|
| [com.yandex.payment.sdk.core.PaymentApi.Payment](-payment/index.md) |  |

## Parameters

core

| | |
|---|---|
| paymentToken | токен корзины для оплаты. |
| orderInfo | информация о заказе. |
| isCredit | будет ли проходить оплата в кредит. Только для работы с Тинькофф кредитами. |
| completion | в коллбек придет либо объект [Payment](-payment/index.md), если старт платежа был успешен или ошибка [PaymentKitError](../../com.yandex.payment.sdk.core.data/-payment-kit-error/index.md) если нет. |
