//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment](../index.md)/[SimplePaymentMediator](index.md)

# SimplePaymentMediator

[datasource]\
class [SimplePaymentMediator](index.md)

Простой медиатор для процесса оплаты. Управляет состоянием кнопки оплаты, экрана и отображением 3ds, и может сам вызывать передачу кода CVC/CVV когда нужно.

## Constructors

| | |
|---|---|
| [SimplePaymentMediator](-simple-payment-mediator.md) | [datasource]<br>fun [SimplePaymentMediator](-simple-payment-mediator.md)() |

## Functions

| Name | Summary |
|---|---|
| [cancel](cancel.md) | [datasource]<br>fun [cancel](cancel.md)()<br>Отменить платёж. |
| [connectUi](connect-ui.md) | [datasource]<br>fun [connectUi](connect-ui.md)(cvnInput: [CvnInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-cvn-input/index.md), webView3ds: [WebView3ds](../../com.yandex.payment.sdk.datasource.bind.interfaces/-web-view3ds/index.md), screen: [PaymentScreen](../../com.yandex.payment.sdk.datasource.payment.interfaces/-payment-screen/index.md))<br>Соединить с UI. |
| [onPayClick](on-pay-click.md) | [datasource]<br>fun [onPayClick](on-pay-click.md)()<br>Обработать нажатие кнопки пользователем. |
| [reset](reset.md) | [datasource]<br>fun [reset](reset.md)()<br>Сбросить состояние и отменить платёж, если активен. |
| [setPaymentProcessing](set-payment-processing.md) | [datasource]<br>fun [setPaymentProcessing](set-payment-processing.md)(processing: [PaymentProcessing](../../com.yandex.payment.sdk.datasource.payment.interfaces/-payment-processing/index.md))<br>Задать API для оплат. |

## Properties

| Name | Summary |
|---|---|
| [paymentCallbacks](payment-callbacks.md) | [datasource]<br>val [paymentCallbacks](payment-callbacks.md): [PaymentCallbacks](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-callbacks/index.md)<br>Коллбеки, которые нужно проставить в [com.yandex.payment.sdk.core.PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md) напрямую или через прослойку. |
