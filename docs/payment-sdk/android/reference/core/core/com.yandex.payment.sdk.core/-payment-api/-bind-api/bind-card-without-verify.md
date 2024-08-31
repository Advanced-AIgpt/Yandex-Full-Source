//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[BindApi](index.md)/[bindCardWithoutVerify](bind-card-without-verify.md)

# bindCardWithoutVerify

[core]\

@MainThread()

~~abstract~~ ~~fun~~ [~~bindCardWithoutVerify~~](bind-card-without-verify.md)~~(~~~~completion~~~~:~~ [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[BoundCard](../../../com.yandex.payment.sdk.core.data/-bound-card/index.md)>~~)~~

Привязать банковскую карточку для пользователя. Используется API V1 (то есть привязка без ввода 3DS кода). **Не рекомендуется** к использованию.

## Parameters

core

| | |
|---|---|
| completion | коллбек с данными о привязке в случае успеха. |
