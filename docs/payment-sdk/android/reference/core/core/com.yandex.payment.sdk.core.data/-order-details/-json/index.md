//[core](../../../../index.md)/[com.yandex.payment.sdk.core.data](../../index.md)/[OrderDetails](../index.md)/[Json](index.md)

# Json

[core]\
data class [Json](index.md)(**data**: String) : [OrderDetails](../index.md)

Параметры заказа в формате JSON для обращения в GooglePay. Будут переданы напрямую в PaymentDataRequest#fromJson.

{% note warning %}

Делайте это только если понимаете, что делаете. Команда PaymentSDK не сможет в данном случае помочь по вопросам получения токена Google Pay.

{% endnote %}

## Constructors

| | |
|---|---|
| [Json](-json.md) | [core]<br>fun [Json](-json.md)(data: String) |

## Properties

| Name | Summary |
|---|---|
| [data](data.md) | [core]<br>val [data](data.md): String<br>описание заказа Google Pay в JSON. |
