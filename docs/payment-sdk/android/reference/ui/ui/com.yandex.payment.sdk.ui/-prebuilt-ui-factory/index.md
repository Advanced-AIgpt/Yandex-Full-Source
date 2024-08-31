//[ui](../../../index.md)/[com.yandex.payment.sdk.ui](../index.md)/[PrebuiltUiFactory](index.md)

# PrebuiltUiFactory

[ui]\
interface [PrebuiltUiFactory](index.md)

Интерфейс фабрики для создания секьюрных Prebuilt UI компонентов, таких как ввод данных банковских карт и отдельно CVV.

## Functions

| Name | Summary |
|---|---|
| [createCardInputView](create-card-input-view.md) | [ui]<br>abstract fun [createCardInputView](create-card-input-view.md)(context: Context, mode: [CardInputMode](../-card-input-mode/index.md), validationConfig: [CardValidationConfig](../../../../core/core/com.yandex.payment.sdk.core.data/-card-validation-config/index.md), cardScanner: [CameraCardScanner](../../com.yandex.payment.sdk.ui.cardscanner/-camera-card-scanner/index.md)?): [CardInputView](../-card-input-view/index.md)<br>Создать вью для ввода банковских карт. |
| [createCvnInputView](create-cvn-input-view.md) | [ui]<br>abstract fun [createCvnInputView](create-cvn-input-view.md)(context: Context): [CvnInputView](../-cvn-input-view/index.md)<br>Создать вью для ввода CVV/CVC кода. |
