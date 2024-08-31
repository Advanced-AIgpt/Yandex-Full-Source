//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getCode](get-code.md)

# getCode

[passport]\

@WorkerThread

@CheckResult

@NonNull

@Deprecated

~~abstract~~ ~~fun~~ [~~getCode~~](get-code.md)~~(~~@NonNulluid: [PassportUid](../-passport-uid/index.md)~~)~~~~:~~ [PassportCode](../-passport-code/index.md)

Возвращает код (&quot;код подтверждения&quot;) по уникальному идентификатору [PassportUid](../-passport-uid/index.md). Действует во многом похоже на вызов getToken, но вместо OAuth токена возвращает код, который потом можно обменять на такой токен. Подробнее смотрите [ в описании серверного API](https://wiki.yandex-team.ru/passport/api/bundle/auth/oauth/#obmensessii/x-tokenanakodpodtverzhdenija) и [документации](https://tech.yandex.ru/oauth/doc/dg/reference/auto-code-client-docpage/).

#### Return

[PassportCode](../-passport-code/index.md) авторизационный код

#### Deprecated

use [getCode](get-code.md) instead.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials](../-passport-credentials/index.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportProperties.Builder](../../../../passport/passport/com.yandex.passport.api/-passport-properties/-builder/add-credentials.md) |  |
| [com.yandex.passport.api.Passport](../-passport/initialize-passport.md) |  |
| [com.yandex.passport.api.PassportApi](authorize-by-code.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | аккаунт с таким uid найден, но валидный токен отсутствует (&quot;мастер токен&quot;, см.выше) |
| [com.yandex.passport.api.exception.PassportCredentialsNotFoundException](../../com.yandex.passport.api.exception/-passport-credentials-not-found-exception/index.md) | при инициализации библиотеки для этого [PassportEnvironment](../-passport-environment/index.md) не был указан [PassportCredentials](../-passport-credentials/index.md) |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\

@WorkerThread

@CheckResult

@NonNull

@Deprecated

~~abstract~~ ~~fun~~ [~~getCode~~](get-code.md)~~(~~@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullcredentials: [PassportCredentials](../-passport-credentials/index.md)~~)~~~~:~~ [PassportCode](../-passport-code/index.md)

Возвращает код (&quot;код подтверждения&quot;) по уникальному идентификатору [PassportUid](../-passport-uid/index.md). Код подтверждения выписывается для clientId переданного в credentials Действует во многом похоже на вызов getToken, но вместо OAuth токена возвращает код, который потом можно обменять на такой токен. Подробнее смотрите [ в описании серверного API](https://wiki.yandex-team.ru/passport/api/bundle/auth/oauth/#obmensessii/x-tokenanakodpodtverzhdenija) и [документации](https://tech.yandex.ru/oauth/doc/dg/reference/auto-code-client-docpage/).

#### Return

[PassportCode](../-passport-code/index.md) авторизационный код

#### Deprecated

use [getCode](get-code.md) instead.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials](../-passport-credentials/index.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportProperties.Builder](../../../../passport/passport/com.yandex.passport.api/-passport-properties/-builder/add-credentials.md) |  |
| [com.yandex.passport.api.Passport](../-passport/initialize-passport.md) |  |
| [com.yandex.passport.api.PassportApi](authorize-by-code.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) |
| credentials | [PassportCredentials](../-passport-credentials/index.md) |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | аккаунт с таким uid найден, но валидный токен отсутствует (&quot;мастер токен&quot;, см.выше) |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [getCode](get-code.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullcredentialProvider: [PassportCredentialProvider](../-passport-credential-provider/index.md)): [PassportCode](../-passport-code/index.md)

Returns the code (&quot;confirmation code&quot;) by a unique identifier [PassportUid](../-passport-uid/index.md). Acts much like calling getToken, but instead of an OAuth token, it returns a code that can then be exchanged for such a token. For details [ in server Api description](https://wiki.yandex-team.ru/passport/api/bundle/auth/oauth/#obmensessii/x-tokenanakodpodtverzhdenija) and [documentation](https://tech.yandex.ru/oauth/doc/dg/reference/auto-code-client-docpage/).

#### Return

[PassportCode](../-passport-code/index.md) auth code

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials](../-passport-credentials/index.md) |  |
| [com.yandex.passport.api.PassportCredentialProvider](../-passport-credential-provider/index.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportProperties.Builder](../../../../passport/passport/com.yandex.passport.api/-passport-properties/-builder/add-credentials.md) |  |
| [com.yandex.passport.api.Passport](../-passport/initialize-passport.md) |  |
| [com.yandex.passport.api.PassportApi](authorize-by-code.md) |  |

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
