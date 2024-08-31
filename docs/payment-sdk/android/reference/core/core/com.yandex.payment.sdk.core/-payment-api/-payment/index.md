//[core](../../../../index.md)/[com.yandex.payment.sdk.core](../../index.md)/[PaymentApi](../index.md)/[Payment](index.md)

# Payment

[core]\
interface [Payment](index.md)

Интерфейс для работы с оплатой. Создаётся после успешного [PaymentApi.startPayment](../start-payment.md). **Важно:** в один момент времени может идти работа только с одним платежом, если нужно провести ещё одну оплату - нужно дождаться завершения предыдущей или сбросить её [cancel](cancel.md).

## Functions

| Name | Summary |
|---|---|
| [cancel](cancel.md) | [core]<br>@MainThread()<br>abstract fun [cancel](cancel.md)()<br>Отменить работу с платежом. |
| [methods](methods.md) | [core]<br>@MainThread()<br>abstract fun [methods](methods.md)(): List<[PaymentMethod](../../../com.yandex.payment.sdk.core.data/-payment-method/index.md)> |
| [pay](pay.md) | [core]<br>@MainThread()<br>abstract fun [pay](pay.md)(method: [PaymentMethod](../../../com.yandex.payment.sdk.core.data/-payment-method/index.md), overrideUserEmail: String?, completion: [PaymentCompletion](../../index.md#152061939%2FClasslikes%2F-2113150450)<[PaymentPollingResult](../../../com.yandex.payment.sdk.core.data/-payment-polling-result/index.md)>)<br>Дослать платёжный метод и начать оплату. |
| [setSbpHandler](set-sbp-handler.md) | [core]<br>@MainThread()<br>abstract fun [setSbpHandler](set-sbp-handler.md)(handler: [SbpHandler](../../../com.yandex.payment.sdk.core.data/-sbp-handler/index.md)?)<br>Задать обработчик для оплаты через Систему Быстрых Платежей. |
| [settings](settings.md) | [core]<br>@MainThread()<br>abstract fun [settings](settings.md)(): [PaymentSettings](../../../com.yandex.payment.sdk.core.data/-payment-settings/index.md) |
| [shouldShowCvv](should-show-cvv.md) | [core]<br>@MainThread()<br>abstract fun [shouldShowCvv](should-show-cvv.md)(cardId: [CardId](../../../com.yandex.payment.sdk.core.data/-card-id/index.md)): Boolean<br>Требуется ли ввод CVV/CVC кода для оплаты данной картой. |
