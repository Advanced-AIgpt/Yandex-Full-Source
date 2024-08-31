//[ui](../../../../index.md)/[com.yandex.payment.sdk.ui.view.payment](../../index.md)/[SelectPaymentAdapter](../index.md)/[PaymentSdkData](index.md)

# PaymentSdkData

[ui]\
data class [PaymentSdkData](index.md)(**method**: [PaymentMethod](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md), **needCvn**: Boolean, **isUnbind**: Boolean) : [SelectPaymentAdapter.Data](../-data/index.md)

Основная реализация данных для отображения - отображает платёжный метод.

## Constructors

| | |
|---|---|
| [PaymentSdkData](-payment-sdk-data.md) | [ui]<br>fun [PaymentSdkData](-payment-sdk-data.md)(method: [PaymentMethod](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md), needCvn: Boolean = false, isUnbind: Boolean = false) |

## Properties

| Name | Summary |
|---|---|
| [isUnbind](is-unbind.md) | [ui]<br>val [isUnbind](is-unbind.md): Boolean = false<br>отображать ли как элемент "отвязка". |
| [method](method.md) | [ui]<br>val [method](method.md): [PaymentMethod](../../../../../core/core/com.yandex.payment.sdk.core.data/-payment-method/index.md)<br>платёжный метод. |
| [needCvn](need-cvn.md) | [ui]<br>val [needCvn](need-cvn.md): Boolean = false<br>отображать ли запрос CVC/CVV кода. |
