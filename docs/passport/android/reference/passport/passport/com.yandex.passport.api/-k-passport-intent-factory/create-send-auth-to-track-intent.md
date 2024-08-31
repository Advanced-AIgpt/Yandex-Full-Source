//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createSendAuthToTrackIntent](create-send-auth-to-track-intent.md)

# createSendAuthToTrackIntent

[passport]\
abstract fun [createSendAuthToTrackIntent](create-send-auth-to-track-intent.md)(context: Context, uri: String, passportUid: [PassportUid](../-passport-uid/index.md)?, useSecureDialogStyle: Boolean): Intent

Экран добавляет авторизацию в трэк который передан в uri Текущий сценарий использования предполагает получение <tt>trackId</tt> с помощью сканера QR кодов на странице [https://passport.yandex.ru/auth?mode=qr](https://passport.yandex.ru/auth?mode=qr) и передачу авторизации пользователя из приложения на эту страницу.<br></br>
