//[ui](../../../../index.md)/[com.yandex.payment.sdk.ui.view.payment](../../index.md)/[PaymentButtonView](../index.md)/[State](index.md)

# State

[ui]\
sealed class [State](index.md)

Состояние кнопки.

## Types

| Name | Summary |
|---|---|
| [Disabled](-disabled/index.md) | [ui]<br>object [Disabled](-disabled/index.md) : [PaymentButtonView.State](index.md)<br>Кнопка недоступна для нажатия. |
| [Enabled](-enabled/index.md) | [ui]<br>class [Enabled](-enabled/index.md)(**mode**: [PaymentButtonView.Mode](../-mode/index.md)) : [PaymentButtonView.State](index.md)<br>Кнопка доступна. |
| [Loading](-loading/index.md) | [ui]<br>object [Loading](-loading/index.md) : [PaymentButtonView.State](index.md)<br>Загрузка, на кнопке отрисовывается троббер. |

## Inheritors

| Name |
|---|
| [PaymentButtonView.State](-disabled/index.md) |
| [PaymentButtonView.State](-enabled/index.md) |
| [PaymentButtonView.State](-loading/index.md) |
