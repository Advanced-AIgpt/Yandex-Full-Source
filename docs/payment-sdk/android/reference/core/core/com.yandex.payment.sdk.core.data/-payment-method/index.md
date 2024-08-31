//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[PaymentMethod](index.md)

# PaymentMethod

[core]\
sealed class [PaymentMethod](index.md)

Платежные методы.

## Types

| Name | Summary |
|---|---|
| [Card](-card/index.md) | [core]<br>data class [Card](-card/index.md)(**id**: [CardId](../-card-id/index.md), **system**: [CardPaymentSystem](../-card-payment-system/index.md), **account**: String, **bankName**: [BankName](../-bank-name/index.md), **familyInfo**: [FamilyInfo](../-family-info/index.md)?) : [PaymentMethod](index.md)<br>Оплата существующей картой. |
| [Cash](-cash/index.md) | [core]<br>object [Cash](-cash/index.md) : [PaymentMethod](index.md)<br>Оплата наличными. |
| [GooglePay](-google-pay/index.md) | [core]<br>object [GooglePay](-google-pay/index.md) : [PaymentMethod](index.md)<br>Оплата через GooglePay. |
| [NewCard](-new-card/index.md) | [core]<br>object [NewCard](-new-card/index.md) : [PaymentMethod](index.md)<br>Оплата новой картой. |
| [Sbp](-sbp/index.md) | [core]<br>object [Sbp](-sbp/index.md) : [PaymentMethod](index.md)<br>Оплата через Систему Быстрых Платежей. |
| [TinkoffCredit](-tinkoff-credit/index.md) | [core]<br>object [TinkoffCredit](-tinkoff-credit/index.md) : [PaymentMethod](index.md)<br>Оплата через кредиты от Тинькофф. |
| [YandexBank](-yandex-bank/index.md) | [core]<br>data class [YandexBank](-yandex-bank/index.md)(**id**: String, **isOwner**: Boolean) : [PaymentMethod](index.md)<br>Оплата через счёт в Яндес-банке. |

## Inheritors

| Name |
|---|
| [PaymentMethod](-card/index.md) |
| [PaymentMethod](-new-card/index.md) |
| [PaymentMethod](-google-pay/index.md) |
| [PaymentMethod](-sbp/index.md) |
| [PaymentMethod](-tinkoff-credit/index.md) |
| [PaymentMethod](-cash/index.md) |
| [PaymentMethod](-yandex-bank/index.md) |
