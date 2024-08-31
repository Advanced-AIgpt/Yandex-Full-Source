//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[sendAuthToTrack](send-auth-to-track.md)

# sendAuthToTrack

[passport]\

@WorkerThread

abstract fun [sendAuthToTrack](send-auth-to-track.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNulltrackId: String)

Авторизовать пользователя по коду trackId в аккаунт [PassportUid](../-passport-uid/index.md). Пользователь должен иметь валидную авторизацию (&quot;мастер токен&quot;). Текущий сценарий использования предполагает получение trackId с помощью сканера QR кодов на странице [https://passport.yandex.ru/auth?mode=qr](https://passport.yandex.ru/auth?mode=qr) и передачу авторизации пользователя из приложения на эту страницу.

## See also

passport

| | |
|---|---|
| com.yandex.passport.api.PassportApi |  |
