//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createTurboAppAuthIntent](create-turbo-app-auth-intent.md)

# createTurboAppAuthIntent

[passport]\
abstract fun [createTurboAppAuthIntent](create-turbo-app-auth-intent.md)(context: Context, properties: [PassportTurboAppAuthProperties](../-passport-turbo-app-auth-properties/index.md)): Intent

Возвращает <tt>Intent</tt> для запуска <tt>Activity</tt> для подтверждения скоупов для турбоаппа. <br></br> Результат приходит в onActivityResult. Для обработки результата используйте [Passport.processTurboAppAuth](../../../passport/com.yandex.passport.api/-passport/process-turbo-app-auth.md)

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportTurboAppAuthProperties](../-passport-turbo-app-auth-properties/index.md) |  |
