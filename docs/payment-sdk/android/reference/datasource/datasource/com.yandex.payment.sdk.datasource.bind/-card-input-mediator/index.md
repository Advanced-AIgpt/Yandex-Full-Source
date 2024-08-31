//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind](../index.md)/[CardInputMediator](index.md)

# CardInputMediator

[datasource]\
abstract class [CardInputMediator](index.md)

Медиатор для работы со вводом даных банковских карт. Содержит в себе базовую логику работы с состояниями вью.

## Constructors

| | |
|---|---|
| [CardInputMediator](-card-input-mediator.md) | [datasource]<br>fun [CardInputMediator](-card-input-mediator.md)() |

## Functions

| Name | Summary |
|---|---|
| [cancel](cancel.md) | [datasource]<br>abstract fun [cancel](cancel.md)()<br>Отменить запущенный после нажатия кнопки процесс. |
| [connectUi](connect-ui.md) | [datasource]<br>@UiThread()<br>open fun [connectUi](connect-ui.md)(cardInput: [CardInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md), button: [CardActionButton](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-action-button/index.md), web3dsView: [WebView3ds](../../com.yandex.payment.sdk.datasource.bind.interfaces/-web-view3ds/index.md), screen: [CardScreen](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-screen/index.md))<br>Состыковать медиатор с UI. |
| [process](process.md) | [datasource]<br>abstract fun [process](process.md)()<br>Обработать нажатие кнопки. |
| [reset](reset.md) | [datasource]<br>@CallSuper()<br>open fun [reset](reset.md)()<br>Отсоединиться от всего UI и выполнить сброс. |

## Properties

| Name | Summary |
|---|---|
| [paymentCallbacks](payment-callbacks.md) | [datasource]<br>val [paymentCallbacks](payment-callbacks.md): [PaymentCallbacks](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-callbacks/index.md)<br>Коллбеки, которые нужно проставить в [com.yandex.payment.sdk.core.PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md) напрямую или через прослойку. |

## Inheritors

| Name |
|---|
| [BindCardMediator](../-bind-card-mediator/index.md) |
| [NewCardPaymentMediator](../../com.yandex.payment.sdk.datasource.payment/-new-card-payment-mediator/index.md) |
