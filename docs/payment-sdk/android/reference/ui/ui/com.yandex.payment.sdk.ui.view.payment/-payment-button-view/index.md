//[ui](../../../index.md)/[com.yandex.payment.sdk.ui.view.payment](../index.md)/[PaymentButtonView](index.md)

# PaymentButtonView

[ui]\
class [PaymentButtonView](index.md)@JvmOverloads()constructor(**context**: Context, **attrs**: AttributeSet?, **defStyleAttr**: Int) : ConstraintLayout

Вью основной кнопки оплаты. Имеет разные состояния, может отображать логотип, а также цены.

## Constructors

| | |
|---|---|
| [PaymentButtonView](-payment-button-view.md) | [ui]<br>@JvmOverloads()<br>fun [PaymentButtonView](-payment-button-view.md)(context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0) |

## Types

| Name | Summary |
|---|---|
| [Mode](-mode/index.md) | [ui]<br>sealed class [Mode](-mode/index.md)<br>Опции для отображения кнопки в состоянии enabled. |
| [State](-state/index.md) | [ui]<br>sealed class [State](-state/index.md)<br>Состояние кнопки. |

## Functions

| Name | Summary |
|---|---|
| [performClick](perform-click.md) | [ui]<br>open override fun [performClick](perform-click.md)(): Boolean |
| [setBrandIcon](set-brand-icon.md) | [ui]<br>fun [setBrandIcon](set-brand-icon.md)(drawable: Drawable)<br>Установить логотип. |
| [setState](set-state.md) | [ui]<br>fun [setState](set-state.md)(state: [PaymentButtonView.State](-state/index.md))<br>Задать состояние кнопки. |
| [setSubTotalTextAppearance](set-sub-total-text-appearance.md) | [ui]<br>fun [setSubTotalTextAppearance](set-sub-total-text-appearance.md)(@StyleRes()resId: Int)<br>Параметры отображения текста с промежуточной ценой. |
| [setText](set-text.md) | [ui]<br>fun [setText](set-text.md)(text: String, totalText: String? = null, subTotalText: String? = null)<br>Задать тексты на кнопке. |
| [setTextAppearance](set-text-appearance.md) | [ui]<br>fun [setTextAppearance](set-text-appearance.md)(@StyleRes()resId: Int)<br>Параметры отображения основного текста. |
| [setTotalTextAppearance](set-total-text-appearance.md) | [ui]<br>fun [setTotalTextAppearance](set-total-text-appearance.md)(@StyleRes()resId: Int)<br>Параметры отображения текста с ценой. |
