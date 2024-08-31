//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment.interfaces](../index.md)/[PaymentProcessing](index.md)

# PaymentProcessing

[datasource]\
interface [PaymentProcessing](index.md)

Интерфейс для API оплаты. Обычно маппится на [com.yandex.payment.sdk.core.PaymentApi.Payment.pay](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/pay.md).

## Functions

| Name | Summary |
|---|---|
| [cancel](cancel.md) | [datasource]<br>abstract fun [cancel](cancel.md)()<br>Отменить оплату. |
| [pay](pay.md) | [datasource]<br>abstract fun [pay](pay.md)(completion: [PaymentCompletion](../../../../core/core/com.yandex.payment.sdk.core/index.md)<[PaymentPollingResult](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-polling-result/index.md)>)<br>Оплатить. |
