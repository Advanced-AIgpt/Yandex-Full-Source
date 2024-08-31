//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getTurboAppAnonymizedUserInfo](get-turbo-app-anonymized-user-info.md)

# getTurboAppAnonymizedUserInfo

[passport]\

@WorkerThread

@NonNull

abstract fun [getTurboAppAnonymizedUserInfo](get-turbo-app-anonymized-user-info.md)(@NonNullproperties: [PassportTurboAppAuthProperties](../-passport-turbo-app-auth-properties/index.md)): [PassportTurboAppJwtToken](../-passport-turbo-app-jwt-token/index.md)

Отдаёт обезличенную информацию о пользователе, включая publisher-specific user id (PSUID). Выдаёт JWT-токен, подписанный секретом приложения.
