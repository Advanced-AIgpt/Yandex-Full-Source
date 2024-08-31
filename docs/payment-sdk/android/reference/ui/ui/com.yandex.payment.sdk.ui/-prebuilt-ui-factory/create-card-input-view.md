//[ui](../../../index.md)/[com.yandex.payment.sdk.ui](../index.md)/[PrebuiltUiFactory](index.md)/[createCardInputView](create-card-input-view.md)

# createCardInputView

[ui]\
abstract fun [createCardInputView](create-card-input-view.md)(context: Context, mode: [CardInputMode](../-card-input-mode/index.md), validationConfig: [CardValidationConfig](../../../../core/core/com.yandex.payment.sdk.core.data/-card-validation-config/index.md), cardScanner: [CameraCardScanner](../../com.yandex.payment.sdk.ui.cardscanner/-camera-card-scanner/index.md)?): [CardInputView](../-card-input-view/index.md)

Создать вью для ввода банковских карт.

#### Return

готовая вью.

## Parameters

ui

| | |
|---|---|
| context | контекст. |
| mode | требуемый вид вью. |
| validationConfig | конфиг для валидации карт. |
| cardScanner | реализация сканера карт, если есть и нужна. |
