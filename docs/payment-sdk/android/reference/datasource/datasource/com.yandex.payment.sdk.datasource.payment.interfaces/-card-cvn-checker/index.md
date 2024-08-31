//[datasource](../../../index.md)/[com.yandex.payment.sdk.datasource.payment.interfaces](../index.md)/[CardCvnChecker](index.md)

# CardCvnChecker

[datasource]\
interface [CardCvnChecker](index.md)

Интерфейс для проверки необходимости ввода CVC/CVV для оплаты конкретной карты. Обычно можно замаппить на [com.yandex.payment.sdk.core.PaymentApi.Payment.shouldShowCvv](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/-payment/should-show-cvv.md).

## Functions

| Name | Summary |
|---|---|
| [needCvn](need-cvn.md) | [datasource]<br>abstract fun [needCvn](need-cvn.md)(card: [PaymentMethod.Card](../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/-card/index.md)): Boolean |
