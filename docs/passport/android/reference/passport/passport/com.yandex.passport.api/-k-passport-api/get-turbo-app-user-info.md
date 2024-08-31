//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getTurboAppUserInfo](get-turbo-app-user-info.md)

# getTurboAppUserInfo

[passport]\
abstract suspend fun [getTurboAppUserInfo](get-turbo-app-user-info.md)(passportEnvironment: [KPassportEnvironment](../-k-passport-environment/index.md), oauthToken: String): Result&lt;[PassportTurboAppJwtToken](../-passport-turbo-app-jwt-token/index.md)&gt;

Отдаёт информацию о пользователе, включая publisher-specific user id (PSUID). Выдаёт JWT-токен, подписанный секретом приложения.

*Exceptions*:

PassportIOException::class, PassportFailedResponseException::class, PassportInvalidTokenException::class, PassportRuntimeUnknownException::class,
