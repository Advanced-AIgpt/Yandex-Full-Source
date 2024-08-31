//[ui](../../../index.md)/[com.yandex.payment.sdk.ui](../index.md)/[CvnInput](index.md)

# CvnInput

[ui]\
interface [CvnInput](index.md)

Интерфейс для работы с логикой ввода CVC/CVV.

## Functions

| Name | Summary |
|---|---|
| [focusInput](focus-input.md) | [ui]<br>abstract fun [focusInput](focus-input.md)()<br>Установить фокус и поднять клавиатуру на поле ввода. |
| [isReady](is-ready.md) | [ui]<br>abstract fun [isReady](is-ready.md)(): Boolean |
| [provideCvn](provide-cvn.md) | [ui]<br>abstract fun [provideCvn](provide-cvn.md)()<br>Передать введённый код в [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md) для продолжения процесса оплаты. |
| [reset](reset.md) | [ui]<br>abstract fun [reset](reset.md)()<br>Сбросить инпуты для ввода. |
| [setCardPaymentSystem](set-card-payment-system.md) | [ui]<br>abstract fun [setCardPaymentSystem](set-card-payment-system.md)(system: [CardPaymentSystem](../../../../core/core/com.yandex.payment.sdk.core.data/-card-payment-system/index.md))<br>Задать платёжную систему. |
| [setOnReadyListener](set-on-ready-listener.md) | [ui]<br>abstract fun [setOnReadyListener](set-on-ready-listener.md)(listener: (Boolean) -> Unit?)<br>Установить листенер корректности введенного кода. |
| [setPaymentApi](set-payment-api.md) | [ui]<br>abstract fun [setPaymentApi](set-payment-api.md)(api: [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md)?)<br>Установить объект [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md). |

## Inheritors

| Name |
|---|
| [CvnInputView](../-cvn-input-view/index.md) |
