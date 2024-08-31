//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[acceptAuthInTrack](accept-auth-in-track.md)

# acceptAuthInTrack

[passport]\

@WorkerThread

abstract fun [acceptAuthInTrack](accept-auth-in-track.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullurl: Uri): Boolean

Подтвердить или отклонить авторизацию в треке

#### Return

true если в url-е было action=accept, false если action=cancel

## Parameters

passport

| | |
|---|---|
| uid | авторизованный пользователь; также используется для определения [PassportEnvironment](../-passport-environment/index.md) |
| url | в url обязательно должен быть хост passport.yandex.ru, а также cgi параметры track_id, secret и action |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportInvalidUrlException](../../com.yandex.passport.api.exception/-passport-invalid-url-exception/index.md) | некорректная ссылка в url или отсутствует один из параметров |
