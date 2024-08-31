//[passport](../../../index.md)/[com.yandex.passport.api.exception](../index.md)/[PassportRuntimeUnknownException](index.md)

# PassportRuntimeUnknownException

[passport]\
open class [PassportRuntimeUnknownException](index.md) : [PassportException](../-passport-exception/index.md)

Во время обработки ответа от контент провайдеров InternalProvider (com.yandex.passport.internal.provider.${applicationId}) и AuthSdkProvider (com.yandex.passport.authsdk.provider.${applicationId}) или обработки запроса внутри этих провайдеров был пойман неизвестный exception. Обязательно нужно логгировать в Метрику все такие случаи. Если приложение может работать без авторизации, нужно как-то показать пользователю, что из-за внутренней ошибки её нет, и продолжать работать. Если приложение не может работать без авторизации, можно падать. Также всегда нужно падать в debuggable сборках.

## Constructors

| | |
|---|---|
| [PassportRuntimeUnknownException](-passport-runtime-unknown-exception.md) | [passport]<br>open fun [PassportRuntimeUnknownException](-passport-runtime-unknown-exception.md)(@NonNullmessage: String) |
| [PassportRuntimeUnknownException](-passport-runtime-unknown-exception.md) | [passport]<br>open fun [PassportRuntimeUnknownException](-passport-runtime-unknown-exception.md)(@NonNullcause: Throwable) |

## Inheritors

| Name |
|---|
| [PassportPaymentAuthRequiredException](../-passport-payment-auth-required-exception/index.md) |
