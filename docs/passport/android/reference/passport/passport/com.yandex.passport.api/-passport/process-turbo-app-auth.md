//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[Passport](index.md)/[processTurboAppAuth](process-turbo-app-auth.md)

# processTurboAppAuth

[passport]\

@NonNull

open fun [processTurboAppAuth](process-turbo-app-auth.md)(@Nullableintent: Intent): [PassportTurboAppResult](../-passport-turbo-app-result/index.md)

Обрабатывает результат экрана OAuth скоупов. В случае, если пользоватль принял скоупы возвращает PassportToken.  Если пользователь Отменил выдачу токена, то придет PassportTurboAppAuthException с message=[USER_CANCELLED](../-passport-error-code/-u-s-e-r_-c-a-n-c-e-l-l-e-d.md) Если пользователь Запретил выдачу токена, то придет PassportTurboAppAuthException с message=[USER_DENIED](../-passport-error-code/-u-s-e-r_-d-e-n-i-e-d.md) В процессе работы диалога скоупов могут возникать различные ошибки. Список ошибок приходит в [getFlowErrorCodes](../../../passport/com.yandex.passport.api.exception/-passport-turbo-app-auth-exception/flow-error-codes.md) или в [getFlowErrorCodes](../../../passport/com.yandex.passport.api/-passport-turbo-app-result/flow-error-codes.md) Варианты таких ошибок: 

[UNKNOWN_ERROR](../-passport-error-code/-u-n-k-n-o-w-n_-e-r-r-o-r.md)[NETWORK_ERROR](../-passport-error-code/-n-e-t-w-o-r-k_-e-r-r-o-r.md)https://wiki.yandex-team.ru/oauth/ifaceapi/#spisokobshhixoshibokhttps://wiki.yandex-team.ru/oauth/ifaceapi/#authorizesubmithttps://wiki.yandex-team.ru/oauth/ifaceapi/#authorizecommitk

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportApi](../-passport-api/create-turbo-app-auth-intent.md) |  |
| [com.yandex.passport.api.PassportErrorCode](../-passport-error-code/index.md) |  |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | - исключение в случае если intent null |
| [com.yandex.passport.api.exception.PassportTurboAppAuthException](../../com.yandex.passport.api.exception/-passport-turbo-app-auth-exception/index.md) | - исключение в случае если интент не содержит нужных данных, либо пользователь отклонил запрос |
