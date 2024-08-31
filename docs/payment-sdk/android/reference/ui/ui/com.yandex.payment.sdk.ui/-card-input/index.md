//[ui](../../../index.md)/[com.yandex.payment.sdk.ui](../index.md)/[CardInput](index.md)

# CardInput

[ui]\
interface [CardInput](index.md)

Интерфейс для работы с логикой ввода данных банковских карт.

## Types

| Name | Summary |
|---|---|
| [State](-state/index.md) | [ui]<br>enum [State](-state/index.md) : Enum<[CardInput.State](-state/index.md)> <br>Состояния вью. |

## Functions

| Name | Summary |
|---|---|
| [focusInput](focus-input.md) | [ui]<br>abstract fun [focusInput](focus-input.md)()<br>Установить фокус и поднять клавиатуру на поле ввода. |
| [proceedToCardDetails](proceed-to-card-details.md) | [ui]<br>abstract fun [proceedToCardDetails](proceed-to-card-details.md)()<br>Перейти в состояние с отображением даты и CVC/CVV. |
| [provideCardData](provide-card-data.md) | [ui]<br>abstract fun [provideCardData](provide-card-data.md)()<br>Передать карточные данные в [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md). |
| [reset](reset.md) | [ui]<br>abstract fun [reset](reset.md)()<br>Сбросить все поля ввода. |
| [setCardPaymentSystemListener](set-card-payment-system-listener.md) | [ui]<br>abstract fun [setCardPaymentSystemListener](set-card-payment-system-listener.md)(listener: ([CardPaymentSystem](../../../../core/core/com.yandex.payment.sdk.core.data/-card-payment-system/index.md)) -> Unit?)<br>Установить листенер для определения платёжной системы. |
| [setMaskedCardNumberListener](set-masked-card-number-listener.md) | [ui]<br>abstract fun [setMaskedCardNumberListener](set-masked-card-number-listener.md)(listener: (String?) -> Unit?)<br>Установить листенер для получения маскированного номера карты. |
| [setOnStateChangeListener](set-on-state-change-listener.md) | [ui]<br>abstract fun [setOnStateChangeListener](set-on-state-change-listener.md)(listener: ([CardInput.State](-state/index.md)) -> Unit?)<br>Установить листенер для отслеживания смены состояний. |
| [setPaymentApi](set-payment-api.md) | [ui]<br>abstract fun [setPaymentApi](set-payment-api.md)(api: [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md)?)<br>Установить объект [PaymentApi](../../../../core/core/com.yandex.payment.sdk.core/-payment-api/index.md), в него будут переданы данные после вызова [provideCardData](provide-card-data.md). |
| [setSaveCardOnPayment](set-save-card-on-payment.md) | [ui]<br>abstract fun [setSaveCardOnPayment](set-save-card-on-payment.md)(save: Boolean)<br>Нужно ли сохранять карту после успешной оплаты. |

## Inheritors

| Name |
|---|
| [CardInputView](../-card-input-view/index.md) |
