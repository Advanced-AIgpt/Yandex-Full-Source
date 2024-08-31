//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportPaymentAuthArguments](index.md)

# PassportPaymentAuthArguments

[passport]\
interface [PassportPaymentAuthArguments](index.md) : Parcelable

Аргументы для прохождения платежной авторизации. Если приложение запросило скоуп, который требует прохождения платежной авторизации, то при попытке получить платежный токен приложение получит исключение [com.yandex.passport.api.exception.PassportPaymentAuthRequiredException](../../com.yandex.passport.api.exception/-passport-payment-auth-required-exception/index.md). Для получения токена необходимо сделать: 1. Получить PassportPaymentAuthArguments из вызова [getToken](../-passport-api/get-token.md) или из [createLoginIntent](../-passport-api/create-login-intent.md) 2. Пройти платежную авторизацию для полученных аргументов. 3. Передать аргументы PassportPaymentAuthArguments в метод [getToken](../-passport-api/get-token.md)

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportApi](../-passport-api/get-token.md) |  |
| [com.yandex.passport.api.exception.PassportPaymentAuthRequiredException](../../com.yandex.passport.api.exception/-passport-payment-auth-required-exception/index.md) |  |

## Functions

| Name | Summary |
|---|---|
| [getPaymentAuthContextId](get-payment-auth-context-id.md) | [passport]<br>@NonNull<br>abstract fun [getPaymentAuthContextId](get-payment-auth-context-id.md)(): String |
| [getPaymentAuthUrl](get-payment-auth-url.md) | [passport]<br>@NonNull<br>abstract fun [getPaymentAuthUrl](get-payment-auth-url.md)(): String |
