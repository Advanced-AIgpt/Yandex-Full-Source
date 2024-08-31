//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment](../index.md)/[NewCardPaymentMediator](index.md)

# NewCardPaymentMediator

[datasource]\
open class [NewCardPaymentMediator](index.md) : [CardInputMediator](../../com.yandex.payment.sdk.datasource.bind/-card-input-mediator/index.md)

Медиатор для оплаты новой картой. Дополнительно проверяет, что пользовательский email не пуст - это может быть полезно для анонимных платежей. **Важно:** проверяется только non null, валидность приложение должно контролировать само.

## Constructors

| | |
|---|---|
| [NewCardPaymentMediator](-new-card-payment-mediator.md) | [datasource]<br>fun [NewCardPaymentMediator](-new-card-payment-mediator.md)() |

## Functions

| Name | Summary |
|---|---|
| [cancel](cancel.md) | [datasource]<br>open override fun [cancel](cancel.md)()<br>Отменить запущенный после нажатия кнопки процесс. |
| [onUserEmailChanged](on-user-email-changed.md) | [datasource]<br>fun [onUserEmailChanged](on-user-email-changed.md)(email: String?)<br>Пользователь изменил email. |
| [process](process.md) | [datasource]<br>open override fun [process](process.md)()<br>Обработать нажатие кнопки. |
| [setPaymentProcessing](set-payment-processing.md) | [datasource]<br>fun [setPaymentProcessing](set-payment-processing.md)(payment: [PaymentProcessing](../../com.yandex.payment.sdk.datasource.payment.interfaces/-payment-processing/index.md))<br>Задать API для оплат. |

## Inheritors

| Name |
|---|
| [NewCardPaymentMediatorLive](../-new-card-payment-mediator-live/index.md) |
