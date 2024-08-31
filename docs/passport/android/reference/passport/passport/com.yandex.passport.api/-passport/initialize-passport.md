//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[Passport](index.md)/[initializePassport](initialize-passport.md)

# initializePassport

[passport]\
open fun [initializePassport](initialize-passport.md)(@NonNullcontext: Context, @NonNullpassportProperties: [PassportProperties](../-passport-properties/index.md))

Инициализация API для процесса *:passport* в Application.onCreate(). Имеет смысл вызывать только в этом процессе. Обязательно нужно вызвать этот метод до начала обращения к библиотеке.

if ([isInPassportProcess()](is-in-passport-process.md)) {
    initializePassport(...)
}

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportProperties.Builder.Factory](../-passport-properties/-builder/-factory/index.md) |  |

## Parameters

passport

| | |
|---|---|
| context | Context приложения |
| passportProperties | [PassportProperties](../-passport-properties/index.md) параметры инициализации: зашифрованные client id и secret oauth токена приложения и пр. |
