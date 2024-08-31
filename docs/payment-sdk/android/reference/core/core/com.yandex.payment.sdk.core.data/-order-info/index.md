//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[OrderInfo](index.md)

# OrderInfo

[core]\
data class [OrderInfo](index.md)(**orderTag**: String?, **orderDetails**: [OrderDetails](../-order-details/index.md)?) : Parcelable

Информация о заказе. Как правило используется совместно с Google Pay.

## Constructors

| | |
|---|---|
| [OrderInfo](-order-info.md) | [core]<br>fun [OrderInfo](-order-info.md)(orderTag: String?, orderDetails: [OrderDetails](../-order-details/index.md)?) |

## Properties

| Name | Summary |
|---|---|
| [orderDetails](order-details.md) | [core]<br>val [orderDetails](order-details.md): [OrderDetails](../-order-details/index.md)?<br>дополнительная информация о заказе. |
| [orderTag](order-tag.md) | [core]<br>val [orderTag](order-tag.md): String?<br>тэг заказа. |
