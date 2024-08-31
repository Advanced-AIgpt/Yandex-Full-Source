//[ui](../../../index.md)/[com.yandex.payment.sdk.ui.cardscanner](../index.md)/[CameraScannerResult](index.md)

# CameraScannerResult

[ui]\
interface [CameraScannerResult](index.md)

Интерфейс с коллбеками результата запуска сканнера.

## Functions

| Name | Summary |
|---|---|
| [onCancel](on-cancel.md) | [ui]<br>abstract fun [onCancel](on-cancel.md)()<br>Пользователь отменил сканирование. |
| [onError](on-error.md) | [ui]<br>abstract fun [onError](on-error.md)(error: [CardScannerError](../-card-scanner-error/index.md))<br>Произошла ошибка. |
| [onSuccess](on-success.md) | [ui]<br>abstract fun [onSuccess](on-success.md)(data: [CardScanData](../-card-scan-data/index.md))<br>Успех. |
