//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[Payment](index.md)/[pay](pay.md)

# pay

[core]\

@MainThread()

abstract fun [pay](pay.md)(method: [PaymentMethod](../../../com.yandex.payment.sdk.core.data/-payment-method/index.md), overrideUserEmail: String?, completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[PaymentPollingResult](../../../com.yandex.payment.sdk.core.data/-payment-polling-result/index.md)>)

Дослать платёжный метод и начать оплату. Если будет передан метод, недоступный для оплаты в данный момент, коллбек [completion](pay.md) будет вызван с ошибкой.

## Parameters

core

| | |
|---|---|
| method | выбранный платёжный метод. |
| overrideUserEmail | опционально использовать другой email для проведения оплаты. |
| completion | коллбек со статусом платежа. |
