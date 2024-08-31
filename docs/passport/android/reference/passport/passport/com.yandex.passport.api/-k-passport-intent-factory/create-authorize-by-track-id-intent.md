//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createAuthorizeByTrackIdIntent](create-authorize-by-track-id-intent.md)

# createAuthorizeByTrackIdIntent

[passport]\
abstract fun [createAuthorizeByTrackIdIntent](create-authorize-by-track-id-intent.md)(context: Context, loginProperties: [PassportLoginProperties](../-passport-login-properties/index.md), trackId: [PassportTrackId](../-passport-track-id/index.md)): Intent

Экран запускает авторизацию по trackId. Показывает экран подтверждения входа

В Activity.onActivityResult возвращается PassportLoginResult если пользователь подтвердил авторизацию.
