//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[GooglePayData](index.md)

# GooglePayData

[core]\
sealed class [GooglePayData](index.md) : Parcelable

Sealed классы для настроек Google Pay.

## Types

| Name | Summary |
|---|---|
| [Direct](-direct/index.md) | [core]<br>data class [Direct](-direct/index.md)(**publicKey**: String) : [GooglePayData](index.md)<br>Настройки для GooglePay в режиме Direct. |
| [Gateway](-gateway/index.md) | [core]<br>data class [Gateway](-gateway/index.md)(**gatewayId**: String, **gatewayMerchantId**: String) : [GooglePayData](index.md)<br>Настройки для GooglePay в режиме Payment Gateway. |

## Inheritors

| Name |
|---|
| [GooglePayData](-gateway/index.md) |
| [GooglePayData](-direct/index.md) |
