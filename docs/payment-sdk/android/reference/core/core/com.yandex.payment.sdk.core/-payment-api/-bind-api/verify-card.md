//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[BindApi](index.md)/[verifyCard](verify-card.md)

# verifyCard

[core]\

@MainThread()

abstract fun [verifyCard](verify-card.md)(cardId: [CardId](../../../com.yandex.payment.sdk.core.data/-card-id/index.md), completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[BoundCard](../../../com.yandex.payment.sdk.core.data/-bound-card/index.md)>)

Выполнить верификационный платёж по карте. Поддерживается только верификация через 3ds.

## Parameters

core

| | |
|---|---|
| cardId | идентификатор карты. |
| completion | коллбек с результатом. |
