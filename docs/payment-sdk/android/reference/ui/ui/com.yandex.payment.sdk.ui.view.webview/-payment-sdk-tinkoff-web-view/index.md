//[ui](../../../index.md)/[com.yandex.payment.sdk.ui.view.webview](../index.md)/[PaymentSdkTinkoffWebView](index.md)

# PaymentSdkTinkoffWebView

[ui]\
class [PaymentSdkTinkoffWebView](index.md)@JvmOverloads()constructor(**context**: Context, **attrs**: AttributeSet?) : [PaymentSdkWebView](../-payment-sdk-web-view/index.md)

Вью для отображения страничек кредитов Тинькофф. Вызовите [init](init.md) для начала работы и потом loadUrl.

## Constructors

| | |
|---|---|
| [PaymentSdkTinkoffWebView](-payment-sdk-tinkoff-web-view.md) | [ui]<br>@JvmOverloads()<br>fun [PaymentSdkTinkoffWebView](-payment-sdk-tinkoff-web-view.md)(context: Context, attrs: AttributeSet? = null) |

## Types

| Name | Summary |
|---|---|
| [TinkoffCreditCallback](-tinkoff-credit-callback/index.md) | [ui]<br>interface [TinkoffCreditCallback](-tinkoff-credit-callback/index.md)<br>Интерфейс коллбека для отслеживания состояния странички с кредитом. |
| [TinkoffState](-tinkoff-state/index.md) | [ui]<br>enum [TinkoffState](-tinkoff-state/index.md) : Enum<[PaymentSdkTinkoffWebView.TinkoffState](-tinkoff-state/index.md)> <br>Перечисление возможных состояний кредита. |

## Functions

| Name | Summary |
|---|---|
| [init](init.md) | [ui]<br>open override fun [init](init.md)(environment: [PaymentSdkEnvironment](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-sdk-environment/index.md))<br>Инициализировать вью в соответствии с заданным окружением. |
| [setCallback](set-callback.md) | [ui]<br>fun [setCallback](set-callback.md)(callback: [PaymentSdkTinkoffWebView.TinkoffCreditCallback](-tinkoff-credit-callback/index.md))<br>Установить коллбэк для отслеживания состояния странички с кредитом. |
