//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createAuthorizationByQrIntent](create-authorization-by-qr-intent.md)

# createAuthorizationByQrIntent

[passport]\

@AnyThread

@CheckResult

@NonNull

@Deprecated

~~abstract~~ ~~fun~~ [~~createAuthorizationByQrIntent~~](create-authorization-by-qr-intent.md)~~(~~@NonNullcontext: Context, @NonNullenvironment: [PassportEnvironment](../-passport-environment/index.md), @NonNulltheme: [PassportTheme](../-passport-theme/index.md)~~)~~~~:~~ Intent

#### Deprecated

Используйте [createAuthorizationByQrIntent](create-authorization-by-qr-intent.md)

[passport]\

@AnyThread

@CheckResult

@NonNull

abstract fun [createAuthorizationByQrIntent](create-authorization-by-qr-intent.md)(@NonNullcontext: Context, @NonNullproperties: [PassportAuthByQrProperties](../-passport-auth-by-qr-properties/index.md)): Intent

Возвращает Intent для запуска Activity с авторизацией по QR коду на Android TV В onActivityResult возвращается [PassportLoginResult](../-passport-login-result/index.md) если пользователь авторизовался. Если пользовать нажал &quot;Пропустить&quot;, то приходит resultCode = [RESULT_SKIP](../-passport/-r-e-s-u-l-t_-s-k-i-p.md)
