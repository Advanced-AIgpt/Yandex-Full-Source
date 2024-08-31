//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[GooglePayApi](index.md)/[isGooglePayAvailable](is-google-pay-available.md)

# isGooglePayAvailable

[core]\

@MainThread()

abstract fun [isGooglePayAvailable](is-google-pay-available.md)(completion: (Boolean) -> Unit)

Проверить, доступен ли на устройстве GooglePay для оплаты. Проверяет только само устройство, на самом сервисе оплата через GooglePay может быть недоступна и нужно использовать [PaymentApi.paymentMethods](../payment-methods.md).

## Parameters

core

| | |
|---|---|
| completion | коллбек с результатом. |
