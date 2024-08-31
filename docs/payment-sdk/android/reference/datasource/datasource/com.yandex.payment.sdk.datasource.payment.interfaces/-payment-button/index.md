//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment.interfaces](../index.md)/[PaymentButton](index.md)

# PaymentButton

[datasource]\
interface [PaymentButton](index.md)

Интерфейс кнопки оплаты.

## Types

| Name | Summary |
|---|---|
| [DisableReason](-disable-reason/index.md) | [datasource]<br>enum [DisableReason](-disable-reason/index.md) : Enum<[PaymentButton.DisableReason](-disable-reason/index.md)> <br>Причина, по которой кнопка недоступна. |
| [State](-state/index.md) | [datasource]<br>sealed class [State](-state/index.md)<br>Состояние кнопки. |

## Functions

| Name | Summary |
|---|---|
| [setState](set-state.md) | [datasource]<br>abstract fun [setState](set-state.md)(state: [PaymentButton.State](-state/index.md))<br>Установить новое состояние. |
