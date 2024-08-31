//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment](../index.md)/[NewCardPaymentMediatorLive](index.md)

# NewCardPaymentMediatorLive

[datasource]\
class [NewCardPaymentMediatorLive](index.md) : [NewCardPaymentMediator](../-new-card-payment-mediator/index.md)

Разновидность медиатора для оплаты новой картой, предназначенная для интеграции во ViewModel. Для использования нужно соединить с вью ввода карточных данных (можно через [CardInputBridge](../../com.yandex.payment.sdk.datasource.bind/-card-input-bridge/index.md)) и подписаться на лайвдаты с состояниями.

## Constructors

| | |
|---|---|
| [NewCardPaymentMediatorLive](-new-card-payment-mediator-live.md) | [datasource]<br>fun [NewCardPaymentMediatorLive](-new-card-payment-mediator-live.md)() |

## Functions

| Name | Summary |
|---|---|
| [connectUi](connect-ui.md) | [datasource]<br>@UiThread()<br>fun [connectUi](connect-ui.md)(cardInput: [CardInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md))<br>Соединить с UI для ввода карточных данных.<br>[datasource]<br>@UiThread()<br>open override fun [connectUi](connect-ui.md)(cardInput: [CardInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md), button: [CardActionButton](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-action-button/index.md), web3dsView: [WebView3ds](../../com.yandex.payment.sdk.datasource.bind.interfaces/-web-view3ds/index.md), screen: [CardScreen](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-screen/index.md))<br>Состыковать медиатор с UI. |
| [getButtonState](get-button-state.md) | [datasource]<br>fun [getButtonState](get-button-state.md)(): LiveData<[CardActionButton.State](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-action-button/-state/index.md)><br>Лайвдата с состоянием кнопки привязки. |
| [getScreenState](get-screen-state.md) | [datasource]<br>fun [getScreenState](get-screen-state.md)(): LiveData<[CardScreen.State](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-screen/-state/index.md)><br>Лайвдата с состоянием экрана. |
| [getWebViewState](get-web-view-state.md) | [datasource]<br>fun [getWebViewState](get-web-view-state.md)(): LiveData<[WebView3ds.State](../../com.yandex.payment.sdk.datasource.bind.interfaces/-web-view3ds/-state/index.md)><br>Лайвдата с состоянием вью 3ds. |
| [onPaymentClick](on-payment-click.md) | [datasource]<br>@UiThread()<br>fun [onPaymentClick](on-payment-click.md)()<br>Вызвать при нажатии кнопки пользователем. |
