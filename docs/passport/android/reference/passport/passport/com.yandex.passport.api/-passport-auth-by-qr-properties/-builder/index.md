//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportAuthByQrProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>open class [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportAuthByQrProperties](../index.md) |
| [setEnvironment](set-environment.md) | [passport]<br>abstract fun [setEnvironment](set-environment.md)(@NonNullenvironment: [PassportEnvironment](../../-passport-environment/index.md)): [PassportAuthByQrProperties.Builder](index.md)<br>Окружение в котором должна происходить авторизация. |
| [setFinishWithoutDialogOnError](set-finish-without-dialog-on-error.md) | [passport]<br>abstract fun [setFinishWithoutDialogOnError](set-finish-without-dialog-on-error.md)(finishWithoutDialogOnError: Boolean): [PassportAuthByQrProperties.Builder](index.md)<br>Включает логику, когда надо не показывать ошибки силами SDK, а обработать её после завершения активити авторизации с результатом [RESULT_ERROR](../../-passport/-r-e-s-u-l-t_-e-r-r-o-r.md). |
| [setOrigin](set-origin.md) | [passport]<br>abstract fun [setOrigin](set-origin.md)(origin: String): [PassportAuthByQrProperties.Builder](index.md)<br>Добавляет параметр origin в URL страницы в WebView. |
| [setShowSettingsButton](set-show-settings-button.md) | [passport]<br>abstract fun [setShowSettingsButton](set-show-settings-button.md)(showSettingsButton: Boolean): [PassportAuthByQrProperties.Builder](index.md)<br>Включает отображение кнопки &quot;Настройки&quot; и текста с предложением открыть настройки на экране с ошибкой. |
| [setShowSkipButton](set-show-skip-button.md) | [passport]<br>abstract fun [setShowSkipButton](set-show-skip-button.md)(showSkipButton: Boolean): [PassportAuthByQrProperties.Builder](index.md)<br>Включает отображение кнопки &quot;Пропустить&quot;. |
| [setTheme](set-theme.md) | [passport]<br>abstract fun [setTheme](set-theme.md)(@NonNulltheme: [PassportTheme](../../-passport-theme/index.md)): [PassportAuthByQrProperties.Builder](index.md) |
