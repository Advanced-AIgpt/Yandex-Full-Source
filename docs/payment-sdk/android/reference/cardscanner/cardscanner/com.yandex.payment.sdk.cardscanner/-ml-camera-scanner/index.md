//[cardscanner](../../../index.md)/[com.yandex.payment.sdk.cardscanner](../index.md)/[MlCameraScanner](index.md)

# MlCameraScanner

[cardscanner]\
class [MlCameraScanner](index.md)(**activity**: ComponentActivity, @StyleRes()**themeId**: Int, **resultCaller**: ActivityResultCaller) : [CameraCardScanner](../../../../ui/ui/com.yandex.payment.sdk.ui.cardscanner/-camera-card-scanner/index.md)

Класс сканера данных банковских карт, основанный на распознавании текста через GooglePlay сервисы.

## Parameters

cardscanner

| | |
|---|---|
| activity | активити. |
| themeId | идентификатор темы для активити сканнера. |
| resultCaller | получатель activity result. |

## Constructors

| | |
|---|---|
| [MlCameraScanner](-ml-camera-scanner.md) | [cardscanner]<br>fun [MlCameraScanner](-ml-camera-scanner.md)(fragment: Fragment, @StyleRes()themeId: Int = R.style.PaymentsdkYaTheme_CardScanner)<br>Альтернативный конструктор для пуска из фрагмента. |
| [MlCameraScanner](-ml-camera-scanner.md) | [cardscanner]<br>fun [MlCameraScanner](-ml-camera-scanner.md)(activity: ComponentActivity, @StyleRes()themeId: Int = R.style.PaymentsdkYaTheme_CardScanner, resultCaller: ActivityResultCaller = activity)<br>активити. |

## Functions

| Name | Summary |
|---|---|
| [isAvailable](is-available.md) | [cardscanner]<br>open override fun [isAvailable](is-available.md)(): Boolean |
| [launch](launch.md) | [cardscanner]<br>open override fun [launch](launch.md)(callback: [CameraScannerResult](../../../../ui/ui/com.yandex.payment.sdk.ui.cardscanner/-camera-scanner-result/index.md)) |
