//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[sendAuthToTrack](send-auth-to-track.md)

# sendAuthToTrack

[passport]\
abstract suspend fun [sendAuthToTrack](send-auth-to-track.md)(uid: [PassportUid](../-passport-uid/index.md), trackId: String): Result&lt;Unit&gt;

Авторизовать пользователя по коду <tt>trackId</tt> в аккаунт [PassportUid](../-passport-uid/index.md).<br></br> Пользователь должен иметь валидную авторизацию (&quot;мастер токен&quot;).<br></br> Текущий сценарий использования предполагает получение <tt>trackId</tt> с помощью сканера QR кодов на странице [https://passport.yandex.ru/auth?mode=qr](https://passport.yandex.ru/auth?mode=qr) и передачу авторизации пользователя из приложения на эту страницу.<br></br>

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportIntentFactory](../-k-passport-intent-factory/create-send-auth-to-track-intent.md) |  |
