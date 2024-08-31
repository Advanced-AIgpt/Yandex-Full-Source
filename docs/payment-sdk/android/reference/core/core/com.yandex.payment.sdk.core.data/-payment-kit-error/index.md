//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[PaymentKitError](index.md)

# PaymentKitError

[core]\
data class [PaymentKitError](index.md)(**kind**: [PaymentKitError.Kind](-kind/index.md), **trigger**: [PaymentKitError.Trigger](-trigger/index.md), **code**: Int?, **status**: String?, **message**: String) : Throwable, Parcelable

Класс ошибок PaymentSDK.

## Constructors

| | |
|---|---|
| [PaymentKitError](-payment-kit-error.md) | [core]<br>fun [PaymentKitError](-payment-kit-error.md)(kind: [PaymentKitError.Kind](-kind/index.md), trigger: [PaymentKitError.Trigger](-trigger/index.md), code: Int?, status: String?, message: String) |

## Types

| Name | Summary |
|---|---|
| [Kind](-kind/index.md) | [core]<br>enum [Kind](-kind/index.md) : Enum<[PaymentKitError.Kind](-kind/index.md)> |
| [Trigger](-trigger/index.md) | [core]<br>enum [Trigger](-trigger/index.md) : Enum<[PaymentKitError.Trigger](-trigger/index.md)> <br>Источники возникновения ошибок. |

## Functions

| Name | Summary |
|---|---|
| [toString](to-string.md) | [core]<br>open override fun [toString](to-string.md)(): String |

## Properties

| Name | Summary |
|---|---|
| [code](code.md) | [core]<br>val [code](code.md): Int?<br>код если известен. |
| [kind](kind.md) | [core]<br>val [kind](kind.md): [PaymentKitError.Kind](-kind/index.md)<br>тип ошибки. |
| [message](message.md) | [core]<br>open override val [message](message.md): String |
| [status](status.md) | [core]<br>val [status](status.md): String?<br>статус если известен. |
| [trigger](trigger.md) | [core]<br>val [trigger](trigger.md): [PaymentKitError.Trigger](-trigger/index.md)<br>источник возникновения. |
