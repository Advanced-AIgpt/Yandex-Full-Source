//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createSendAuthToTrackIntent](create-send-auth-to-track-intent.md)

# createSendAuthToTrackIntent

[passport]\

@AnyThread

@CheckResult

@NonNull

abstract fun [createSendAuthToTrackIntent](create-send-auth-to-track-intent.md)(@NonNullcontext: Context, @NonNulluri: String, @NullablepassportUid: [PassportUid](../-passport-uid/index.md), useSecureDialogStyle: Boolean): Intent

Экран добавляет авторизацию в трэк который передан в uri Текущий сценарий использования предполагает получение trackId с помощью сканера QR кодов на странице [https://passport.yandex.ru/auth?mode=qr](https://passport.yandex.ru/auth?mode=qr) и передачу авторизации пользователя из приложения на эту страницу.
