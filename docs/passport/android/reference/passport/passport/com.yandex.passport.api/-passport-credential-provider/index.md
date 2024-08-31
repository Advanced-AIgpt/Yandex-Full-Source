//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportCredentialProvider](index.md)

# PassportCredentialProvider

[passport]\
interface [PassportCredentialProvider](index.md)

Used to provide passport credentials for some methods where credentials are optional.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials](../-passport-credentials/index.md) |  |

## Types

| Name | Summary |
|---|---|
| [FromProperties](-from-properties/index.md) | [passport]<br>object [FromProperties](-from-properties/index.md) : [PassportCredentialProvider](index.md)<br>[PassportCredentials](../-passport-credentials/index.md) will be taken from [PassportProperties](../-passport-properties/index.md) provided by host in [Passport.initializePassport](../../../../passport/passport/com.yandex.passport.api/-passport/initialize-passport.md). |
| [NoCredentials](-no-credentials/index.md) | [passport]<br>object [NoCredentials](-no-credentials/index.md) : [PassportCredentialProvider](index.md)<br>No credentials should be provided t method. |
| [Provided](-provided/index.md) | [passport]<br>data class [Provided](-provided/index.md)(val passportCredentials: [PassportCredentials](../-passport-credentials/index.md)) : [PassportCredentialProvider](index.md)<br>Directly provided [PassportCredentials](../-passport-credentials/index.md). |

## Inheritors

| Name |
|---|
| [NoCredentials](-no-credentials/index.md) |
| [FromProperties](-from-properties/index.md) |
| [Provided](-provided/index.md) |
