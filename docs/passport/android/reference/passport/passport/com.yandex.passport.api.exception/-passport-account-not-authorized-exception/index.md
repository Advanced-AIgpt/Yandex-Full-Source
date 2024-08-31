//[passport](../../../index.md)/[com.yandex.passport.api.exception](../index.md)/[PassportAccountNotAuthorizedException](index.md)

# PassportAccountNotAuthorizedException

[passport]\
open class [PassportAccountNotAuthorizedException](index.md) : [PassportException](../-passport-exception/index.md)

Аккаунт существует в базе аккаунтов, но токен не валиден. Пользователь может отозвать токен, сменив пароль, или каким-то другим способом. Срок действия токена может истечь. В этой ситуации, вероятно, пользователя нужно отправить на повторную авторизацию. Используйте [selectAccount](../../../passport/com.yandex.passport.api/-passport-login-properties/-builder/select-account.md) и [createLoginIntent](../../com.yandex.passport.api/-passport-api/create-login-intent.md) для создания нужного интента.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../-passport-account-not-found-exception/index.md) |  |
| [com.yandex.passport.api.PassportApi](../../com.yandex.passport.api/-passport-api/create-login-intent.md) |  |
| [com.yandex.passport.api.PassportLoginProperties.Builder](../../../passport/com.yandex.passport.api/-passport-login-properties/-builder/select-account.md) |  |

## Constructors

| | |
|---|---|
| [PassportAccountNotAuthorizedException](-passport-account-not-authorized-exception.md) | [passport]<br>open fun [PassportAccountNotAuthorizedException](-passport-account-not-authorized-exception.md)() |
| [PassportAccountNotAuthorizedException](-passport-account-not-authorized-exception.md) | [passport]<br>open fun [PassportAccountNotAuthorizedException](-passport-account-not-authorized-exception.md)(@NonNulluid: Uid) |
| [PassportAccountNotAuthorizedException](-passport-account-not-authorized-exception.md) | [passport]<br>open fun [PassportAccountNotAuthorizedException](-passport-account-not-authorized-exception.md)(@NonNullcause: Throwable) |

## Functions

| Name | Summary |
|---|---|
| [getUid](get-uid.md) | [passport]<br>@Nullable<br>open fun [getUid](get-uid.md)(): [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md) |
