//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[BindApi](index.md)

# BindApi

[core]\
interface [BindApi](index.md)

Интерфейс для работы с банковскими картами.

## Functions

| Name | Summary |
|---|---|
| [bindCardWithoutVerify](bind-card-without-verify.md) | [core]<br>@MainThread()<br>~~abstract~~ ~~fun~~ [~~bindCardWithoutVerify~~](bind-card-without-verify.md)~~(~~~~completion~~~~:~~ [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[BoundCard](../../../com.yandex.payment.sdk.core.data/-bound-card/index.md)>~~)~~<br>Привязать банковскую карточку для пользователя. |
| [bindCardWithVerify](bind-card-with-verify.md) | [core]<br>@MainThread()<br>abstract fun [bindCardWithVerify](bind-card-with-verify.md)(completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[BoundCard](../../../com.yandex.payment.sdk.core.data/-bound-card/index.md)>)<br>Привязать банковскую карточку для пользователя. |
| [cancel](cancel.md) | [core]<br>@MainThread()<br>abstract fun [cancel](cancel.md)()<br>Отменить процесс привязки или верификации. |
| [unbindCard](unbind-card.md) | [core]<br>@MainThread()<br>abstract fun [unbindCard](unbind-card.md)(cardId: [CardId](../../../com.yandex.payment.sdk.core.data/-card-id/index.md), completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<Unit>)<br>Отвязать карту. |
| [verifyCard](verify-card.md) | [core]<br>@MainThread()<br>abstract fun [verifyCard](verify-card.md)(cardId: [CardId](../../../com.yandex.payment.sdk.core.data/-card-id/index.md), completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[BoundCard](../../../com.yandex.payment.sdk.core.data/-bound-card/index.md)>)<br>Выполнить верификационный платёж по карте. |
