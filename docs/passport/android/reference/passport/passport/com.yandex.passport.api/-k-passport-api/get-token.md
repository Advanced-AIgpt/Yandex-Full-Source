//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getToken](get-token.md)

# getToken

[passport]\
abstract suspend fun [getToken](get-token.md)(uid: [PassportUid](../-passport-uid/index.md)): Result&lt;[PassportToken](../-passport-token/index.md)&gt;

Возвращает авторизационный токен по уникальному идентификатору [PassportUid](../-passport-uid/index.md).<br></br><br></br> Этот токен имеет набор прав (scope), соответствующие OAuth приложению – паре client id / client secret из [PassportCredentials](../-passport-credentials/index.md), указанному при инициализации библиотеки в методе [Passport.initializePassport](../../../../passport/passport/com.yandex.passport.api/-passport/initialize-passport.md) для соответствующего окружения [PassportEnvironment](../-passport-environment/index.md).<br></br><br></br> Для описания этого токена можно также встретить название &quot;клиентский токен&quot;, в противоположность &quot;мастер токену&quot;, права которого позволяют получать другие токены, но не содержат &quot;клиентские&quot;. Такие токены используются внутри библиотеки.<br></br><br></br> В случае, если сервер ответил, что данный токен больше не валиден, нужно его удалить, вызвав метод [dropToken](drop-token.md), до запроса нового токена.<br></br>

#### Return

[PassportToken](../-passport-token/index.md) авторизационный токен

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials](../-passport-credentials/index.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../-passport-environment/index.md) |  |
| [com.yandex.passport.api.PassportProperties.Builder](../-passport-properties/-builder/add-credentials.md) |  |
| [com.yandex.passport.api.KPassport](../-k-passport/initialize.md) |  |
| [com.yandex.passport.api.KPassportApi](get-code.md) |  |

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
| [com.yandex.passport.api.exception.PassportPaymentAuthRequiredException](../../com.yandex.passport.api.exception/-passport-payment-auth-required-exception/index.md) | для получения токена необходимо прохождение платежной авторизации. Данное исключение приходит только для приложений у которых объявлен специальный денежный скоуп. Для остальных приложений данное исключение можно обрабатывать как PassportRuntimeUnknownException |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\
abstract suspend fun [getToken](get-token.md)(uid: [PassportUid](../-passport-uid/index.md), arguments: [PassportPaymentAuthArguments](../-passport-payment-auth-arguments/index.md)): Result&lt;[PassportToken](../-passport-token/index.md)&gt;

Возвращает авторизационный токен по уникальному идентификатору [PassportUid](../-passport-uid/index.md).<br></br>.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportApi](get-token.md) |  |

## Parameters

passport

| | |
|---|---|
| arguments | [PassportPaymentAuthArguments](../-passport-payment-auth-arguments/index.md) |

[passport]\
abstract suspend fun [getToken](get-token.md)(uid: [PassportUid](../-passport-uid/index.md), credentials: [PassportCredentials](../-passport-credentials/index.md)): Result&lt;[PassportToken](../-passport-token/index.md)&gt;

Возвращает авторизационный токен по уникальному идентификатору [PassportUid](../-passport-uid/index.md).<br></br>.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportApi](get-token.md) |  |

## Parameters

passport

| | |
|---|---|
| credentials | [PassportCredentials](../-passport-credentials/index.md) - если нужно выписать токен на client_id отличный от указанных при инициализации библиотеки |
