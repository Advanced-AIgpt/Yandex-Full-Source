//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind.interfaces](../index.md)/[WebView3ds](index.md)

# WebView3ds

[datasource]\
interface [WebView3ds](index.md)

Интерфейс для отображения 3ds. Вы можете интегрировать его как угодно в приложении - через отдельный фрагмент, вью и тд. Для непосредственного отображения можно использовать [com.yandex.payment.sdk.ui.view.webview.PaymentSdkWebView](../../../../ui/ui/com.yandex.payment.sdk.ui.view.webview/-payment-sdk-web-view/index.md).

## Types

| Name | Summary |
|---|---|
| [State](-state/index.md) | [datasource]<br>sealed class [State](-state/index.md)<br>Состояние 3ds. |

## Functions

| Name | Summary |
|---|---|
| [setState](set-state.md) | [datasource]<br>abstract fun [setState](set-state.md)(state: [WebView3ds.State](-state/index.md))<br>Задать состояние отображения 3ds. |
