//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind](../index.md)/[BindCardMediatorLive](index.md)/[connectUi](connect-ui.md)

# connectUi

[datasource]\

@UiThread()

open override fun [connectUi](connect-ui.md)(cardInput: [CardInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md), button: [CardActionButton](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-action-button/index.md), web3dsView: [WebView3ds](../../com.yandex.payment.sdk.datasource.bind.interfaces/-web-view3ds/index.md), screen: [CardScreen](../../com.yandex.payment.sdk.datasource.bind.interfaces/-card-screen/index.md))

Состыковать медиатор с UI.

## Parameters

datasource

| | |
|---|---|
| cardInput | реализация вью ввода карточных данных. |
| button | реализация кнопки действия. |
| web3dsView | реализация вью для отображения 3ds. |
| screen | реализация экрана. |

[datasource]\

@UiThread()

fun [connectUi](connect-ui.md)(cardInput: [CardInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md))

Соединить с UI для ввода карточных данных.

## Parameters

datasource

| | |
|---|---|
| cardInput | реализация вью для ввода карточных данных. |
