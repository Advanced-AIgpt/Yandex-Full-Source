//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createTurboAppAuthIntent](create-turbo-app-auth-intent.md)

# createTurboAppAuthIntent

[passport]\

@AnyThread

@CheckResult

@NonNull

abstract fun [createTurboAppAuthIntent](create-turbo-app-auth-intent.md)(@NonNullcontext: Context, @NonNullproperties: [PassportTurboAppAuthProperties](../-passport-turbo-app-auth-properties/index.md)): Intent

Возвращает Intent для запуска Activity для подтверждения скоупов для турбоаппа.  Результат приходит в onActivityResult. Для обработки результата используйте [processTurboAppAuth](../-passport/process-turbo-app-auth.md)

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportTurboAppAuthProperties](../-passport-turbo-app-auth-properties/index.md) |  |
