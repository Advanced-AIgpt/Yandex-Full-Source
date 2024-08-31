//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getTurboAppAnonymizedUserInfo](get-turbo-app-anonymized-user-info.md)

# getTurboAppAnonymizedUserInfo

[passport]\
abstract suspend fun [getTurboAppAnonymizedUserInfo](get-turbo-app-anonymized-user-info.md)(properties: [PassportTurboAppAuthProperties](../-passport-turbo-app-auth-properties/index.md)): Result&lt;[PassportTurboAppJwtToken](../-passport-turbo-app-jwt-token/index.md)&gt;

Отдаёт обезличенную информацию о пользователе, включая publisher-specific user id (PSUID). Выдаёт JWT-токен, подписанный секретом приложения.

*Exceptions*: PassportAccountNotAuthorizedException::class, PassportIOException::class, PassportFailedResponseException::class, PassportAccountNotFoundException::class, PassportRuntimeUnknownException::class,
