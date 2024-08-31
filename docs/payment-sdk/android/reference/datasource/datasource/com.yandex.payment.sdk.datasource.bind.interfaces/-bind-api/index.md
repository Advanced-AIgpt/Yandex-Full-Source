//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind.interfaces](../index.md)/[BindApi](index.md)

# BindApi

[datasource]\
interface [BindApi](index.md)

Интерфейс API для привязок карт. В общем случае можно маппить на [com.yandex.payment.sdk.core.PaymentApi.BindApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/-bind-api/index.md),

## Functions

| Name | Summary |
|---|---|
| [bindCard](bind-card.md) | [datasource]<br>abstract fun [bindCard](bind-card.md)(completion: [PaymentCompletion](../../../../core/core/com.yandex.payment.sdk.core/index.md)<[BoundCard](../../../../core/core/com.yandex.payment.sdk.core.data/-bound-card/index.md)>)<br>Привязать карту. |
| [cancel](cancel.md) | [datasource]<br>abstract fun [cancel](cancel.md)()<br>Отменить процесс привязки. |
