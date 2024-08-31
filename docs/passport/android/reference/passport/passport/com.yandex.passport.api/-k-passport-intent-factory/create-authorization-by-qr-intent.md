//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createAuthorizationByQrIntent](create-authorization-by-qr-intent.md)

# createAuthorizationByQrIntent

[passport]\
abstract fun [createAuthorizationByQrIntent](create-authorization-by-qr-intent.md)(context: Context, properties: [PassportAuthByQrProperties](../-passport-auth-by-qr-properties/index.md)): Intent

Возвращает <tt>Intent</tt> для запуска <tt>Activity</tt> с авторизацией по QR коду на Android TV<br></br>

В Activity.onActivityResult возвращается [PassportLoginResult](../-passport-login-result/index.md) если пользователь авторизовался. Если пользовать нажал &quot;Пропустить&quot;, то приходит resultCode = [Passport.RESULT_SKIP](../-passport/-r-e-s-u-l-t_-s-k-i-p.md)
