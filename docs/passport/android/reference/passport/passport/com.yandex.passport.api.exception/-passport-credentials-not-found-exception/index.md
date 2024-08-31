//[passport](../../../index.md)/[com.yandex.passport.api.exception](../index.md)/[PassportCredentialsNotFoundException](index.md)

# PassportCredentialsNotFoundException

[passport]\
open class [PassportCredentialsNotFoundException](index.md) : [PassportException](../-passport-exception/index.md)

При инициализации библиотеки для этого [PassportEnvironment](../../com.yandex.passport.api/-passport-environment/index.md) не был указан [PassportCredentials](../../com.yandex.passport.api/-passport-credentials/index.md). Возникает при попытке получения клиентского токена или кода по мастер токену.

## See also

passport

| | |
|---|---|
| com.yandex.passport.api.PassportApi |  |

## Constructors

| | |
|---|---|
| [PassportCredentialsNotFoundException](-passport-credentials-not-found-exception.md) | [passport]<br>open fun [PassportCredentialsNotFoundException](-passport-credentials-not-found-exception.md)(@NonNullenvironment: [PassportEnvironment](../../com.yandex.passport.api/-passport-environment/index.md)) |
| [PassportCredentialsNotFoundException](-passport-credentials-not-found-exception.md) | [passport]<br>open fun [PassportCredentialsNotFoundException](-passport-credentials-not-found-exception.md)(@NonNullcause: Throwable) |
