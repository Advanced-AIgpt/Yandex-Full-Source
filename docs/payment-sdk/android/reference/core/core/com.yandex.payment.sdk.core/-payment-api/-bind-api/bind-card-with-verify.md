//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[BindApi](index.md)/[bindCardWithVerify](bind-card-with-verify.md)

# bindCardWithVerify

[core]\

@MainThread()

abstract fun [bindCardWithVerify](bind-card-with-verify.md)(completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[BoundCard](../../../com.yandex.payment.sdk.core.data/-bound-card/index.md)>)

Привязать банковскую карточку для пользователя. Используется API V2 (то есть привязка со вводом 3DS) **Важно:** обязательно должен быть задан валидный сервис токен в [Merchant](../../../com.yandex.payment.sdk.core.data/-merchant/index.md).

## Parameters

core

| | |
|---|---|
| completion | коллбек с данными о привязке в случае успеха. |
