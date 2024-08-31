//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getCode](get-code.md)

# getCode

[passport]\
abstract suspend fun [getCode](get-code.md)(uid: [PassportUid](../-passport-uid/index.md), credentialProvider: [PassportCredentialProvider](../-passport-credential-provider/index.md)): Result&lt;[PassportCode](../-passport-code/index.md)&gt;

Returns the code (&quot;confirmation code&quot;) by a unique identifier [PassportUid](../-passport-uid/index.md).

Acts much like calling .getToken, but instead of an OAuth token, it returns a code that can then be exchanged for such a token.

For details [in server Api description](https://wiki.yandex-team.ru/passport/api/bundle/auth/oauth/#obmensessii/x-tokenanakodpodtverzhdenija) and [documentation](https://tech.yandex.ru/oauth/doc/dg/reference/auto-code-client-docpage/).

#### Return

[PassportCode](../-passport-code/index.md) auth code

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials](../-passport-credentials/index.md) |  |
| [com.yandex.passport.api.PassportCredentialProvider](../-passport-credential-provider/index.md) |  |
| [com.yandex.passport.api.KPassportEnvironment](../-k-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportProperties.Builder](../-passport-properties/-builder/add-credentials.md) |  |
| [com.yandex.passport.api.KPassport](../-k-passport/initialize.md) |  |
| [com.yandex.passport.api.KPassportApi](authorize-by-code.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) |
| credentialProvider | [PassportCredentialProvider](../-passport-credential-provider/index.md) |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | account with given uid is not found |
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | account with given uid is found, but valid token is missing (&quot;master token&quot;, see above) |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | network error, shoud retry |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | internal error |
