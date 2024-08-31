//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[OrderDetails](index.md)

# OrderDetails

[core]\
sealed class [OrderDetails](index.md) : Parcelable

Параметры заказа для GooglePay.

## Types

| Name | Summary |
|---|---|
| [Json](-json/index.md) | [core]<br>data class [Json](-json/index.md)(**data**: String) : [OrderDetails](index.md)<br>Параметры заказа в формате JSON для обращения в GooglePay. |
| [Strict](-strict/index.md) | [core]<br>data class [Strict](-strict/index.md)(**currency**: String, **amount**: BigDecimal?, **priceStatus**: String?, **label**: String?) : [OrderDetails](index.md)<br>Параметры заказа для обращения в GooglePay. |

## Inheritors

| Name |
|---|
| [OrderDetails](-strict/index.md) |
| [OrderDetails](-json/index.md) |
