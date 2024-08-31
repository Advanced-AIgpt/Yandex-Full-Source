//[core](../../../../index.md)/[com.yandex.payment.sdk.core.data](../../index.md)/[OrderDetails](../index.md)/[Strict](index.md)

# Strict

[core]\
data class [Strict](index.md)(**currency**: String, **amount**: BigDecimal?, **priceStatus**: String?, **label**: String?) : [OrderDetails](../index.md)

Параметры заказа для обращения в GooglePay.

## See also

core

| | |
|---|---|
| [com.yandex.payment.sdk.core.PaymentApi.GooglePayApi](../../../com.yandex.payment.sdk.core/-payment-api/-google-pay-api/index.md) |  |

## Constructors

| | |
|---|---|
| [Strict](-strict.md) | [core]<br>fun [Strict](-strict.md)(currency: String, amount: BigDecimal? = null, priceStatus: String? = null, label: String? = null) |

## Properties

| Name | Summary |
|---|---|
| [amount](amount.md) | [core]<br>val [amount](amount.md): BigDecimal? = null<br>сумма платежа, null если неизвестна. |
| [currency](currency.md) | [core]<br>val [currency](currency.md): String<br>валюта платежа (ISO 4217). |
| [label](label.md) | [core]<br>val [label](label.md): String? = null<br>текстовая надпись рядом с итоговой суммой, можно передать null если не нужна. |
| [priceStatus](price-status.md) | [core]<br>val [priceStatus](price-status.md): String? = null<br>статус цены заказа, если передать null будет рассчитан автоматически. |
