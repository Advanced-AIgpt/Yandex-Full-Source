//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[acceptAuthInTrack](accept-auth-in-track.md)

# acceptAuthInTrack

[passport]\
abstract suspend fun [acceptAuthInTrack](accept-auth-in-track.md)(uid: [PassportUid](../-passport-uid/index.md), url: Uri): Result&lt;Boolean&gt;

Подтвердить или отклонить авторизацию в треке<br></br>

*Exceptions:*

PassportIOException::class, PassportFailedResponseException::class, PassportRuntimeUnknownException::class, PassportAccountNotFoundException::class, PassportAccountNotAuthorizedException::class, PassportInvalidUrlException::class

#### Return

<tt>true</tt> если в url-е было action=accept, <tt>false</tt> если action=cancel

## Parameters

passport

| | |
|---|---|
| uid | авторизованный пользователь; также  используется для определения [PassportEnvironment](../-passport-environment/index.md) |
| url | в <tt>url</tt> обязательно должен быть хост passport.yandex.ru, а также cgi параметры <tt>track_id</tt>, <tt>secret</tt> и <tt>action</tt> |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportInvalidUrlException](../../com.yandex.passport.api.exception/-passport-invalid-url-exception/index.md) | некорректная ссылка в <tt>url</tt> или отсутствует один из параметров |
