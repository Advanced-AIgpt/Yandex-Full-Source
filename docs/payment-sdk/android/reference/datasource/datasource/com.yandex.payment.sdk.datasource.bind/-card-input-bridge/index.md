//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.bind](../index.md)/[CardInputBridge](index.md)

# CardInputBridge

[datasource]\
class [CardInputBridge](index.md) : [CardInput](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/index.md)

Вспомогательный класс бриджа для ввода карточных данных, чтобы не тянуть прямую зависимость от вьюшки туда, куда не следует.

## Constructors

| | |
|---|---|
| [CardInputBridge](-card-input-bridge.md) | [datasource]<br>fun [CardInputBridge](-card-input-bridge.md)() |

## Functions

| Name | Summary |
|---|---|
| [focusInput](focus-input.md) | [datasource]<br>open override fun [focusInput](focus-input.md)() |
| [proceedToCardDetails](proceed-to-card-details.md) | [datasource]<br>open override fun [proceedToCardDetails](proceed-to-card-details.md)() |
| [provideCardData](provide-card-data.md) | [datasource]<br>open override fun [provideCardData](provide-card-data.md)() |
| [reset](reset.md) | [datasource]<br>open override fun [reset](reset.md)() |
| [setCardPaymentSystemListener](set-card-payment-system-listener.md) | [datasource]<br>open override fun [setCardPaymentSystemListener](set-card-payment-system-listener.md)(listener: ([CardPaymentSystem](../../../../core/core/com.yandex.payment.sdk.core.data/-card-payment-system/index.md)) -> Unit?) |
| [setMaskedCardNumberListener](set-masked-card-number-listener.md) | [datasource]<br>open override fun [setMaskedCardNumberListener](set-masked-card-number-listener.md)(listener: (String?) -> Unit?) |
| [setOnStateChangeListener](set-on-state-change-listener.md) | [datasource]<br>open override fun [setOnStateChangeListener](set-on-state-change-listener.md)(listener: ([CardInput.State](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input/-state/index.md)) -> Unit?) |
| [setPaymentApi](set-payment-api.md) | [datasource]<br>open override fun [setPaymentApi](set-payment-api.md)(api: [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md)?) |
| [setSaveCardOnPayment](set-save-card-on-payment.md) | [datasource]<br>open override fun [setSaveCardOnPayment](set-save-card-on-payment.md)(save: Boolean) |

## Properties

| Name | Summary |
|---|---|
| [cardInputView](card-input-view.md) | [datasource]<br>var [cardInputView](card-input-view.md): [CardInputView](../../../../ui/ui/com.yandex.payment.sdk.ui/-card-input-view/index.md)? = null<br>Установить вью ввода карточных данных. |
