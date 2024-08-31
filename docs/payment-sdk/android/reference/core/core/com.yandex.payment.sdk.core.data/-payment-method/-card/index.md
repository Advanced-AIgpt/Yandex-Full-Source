//[core](../../../../index.md)/[com.yandex.payment.sdk.core.data](../../index.md)/[PaymentMethod](../index.md)/[Card](index.md)

# Card

[core]\
data class [Card](index.md)(**id**: [CardId](../../-card-id/index.md), **system**: [CardPaymentSystem](../../-card-payment-system/index.md), **account**: String, **bankName**: [BankName](../../-bank-name/index.md), **familyInfo**: [FamilyInfo](../../-family-info/index.md)?) : [PaymentMethod](../index.md)

Оплата существующей картой.

## Constructors

| | |
|---|---|
| [Card](-card.md) | [core]<br>fun [Card](-card.md)(id: [CardId](../../-card-id/index.md), system: [CardPaymentSystem](../../-card-payment-system/index.md), account: String, bankName: [BankName](../../-bank-name/index.md), familyInfo: [FamilyInfo](../../-family-info/index.md)?) |

## Properties

| Name | Summary |
|---|---|
| [account](account.md) | [core]<br>val [account](account.md): String<br>маскированный номер. |
| [bankName](bank-name.md) | [core]<br>val [bankName](bank-name.md): [BankName](../../-bank-name/index.md)<br>название банка. |
| [familyInfo](family-info.md) | [core]<br>val [familyInfo](family-info.md): [FamilyInfo](../../-family-info/index.md)? |
| [id](id.md) | [core]<br>val [id](id.md): [CardId](../../-card-id/index.md)<br>идентификатор карты. |
| [system](system.md) | [core]<br>val [system](system.md): [CardPaymentSystem](../../-card-payment-system/index.md)<br>платежная система. |
