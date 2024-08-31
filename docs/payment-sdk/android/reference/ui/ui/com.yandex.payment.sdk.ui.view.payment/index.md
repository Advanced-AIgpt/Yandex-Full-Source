//[ui](../../index.md)/[com.yandex.payment.sdk.ui.view.payment](index.md)

# Package com.yandex.payment.sdk.ui.view.payment

## Types

| Name | Summary |
|---|---|
| [PaymentButtonView](-payment-button-view/index.md) | [ui]<br>class [PaymentButtonView](-payment-button-view/index.md)@JvmOverloads()constructor(**context**: Context, **attrs**: AttributeSet?, **defStyleAttr**: Int) : ConstraintLayout<br>Вью основной кнопки оплаты. |
| [SelectPaymentAdapter](-select-payment-adapter/index.md) | [ui]<br>open class [SelectPaymentAdapter](-select-payment-adapter/index.md)(**listener**: [SelectPaymentAdapter.PaymentMethodClickListener](-select-payment-adapter/-payment-method-click-listener/index.md), **prebuiltUiFactory**: [PrebuiltUiFactory](../com.yandex.payment.sdk.ui/-prebuilt-ui-factory/index.md), **isLightTheme**: Boolean, **mode**: [SelectPaymentAdapter.AdapterMode](-select-payment-adapter/-adapter-mode/index.md)) : RecyclerView.Adapter<[SelectPaymentAdapter.BaseViewHolder](-select-payment-adapter/-base-view-holder/index.md)> <br>Адаптер для работы со списком платёжных методов. |

## Functions

| Name | Summary |
|---|---|
| [getPaymentMethod](get-payment-method.md) | [ui]<br>fun [SelectPaymentAdapter.Data](-select-payment-adapter/-data/index.md)?.[getPaymentMethod](get-payment-method.md)(): [PaymentMethod](../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md)<br>Вспомогательный метод для конвертации дефолтной [SelectPaymentAdapter.Data](-select-payment-adapter/-data/index.md) в [PaymentMethod](../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md). |
