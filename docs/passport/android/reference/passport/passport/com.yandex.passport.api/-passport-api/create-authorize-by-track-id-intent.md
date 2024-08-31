//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createAuthorizeByTrackIdIntent](create-authorize-by-track-id-intent.md)

# createAuthorizeByTrackIdIntent

[passport]\

@AnyThread

@CheckResult

@NonNull

abstract fun [createAuthorizeByTrackIdIntent](create-authorize-by-track-id-intent.md)(@NonNullcontext: Context, @NonNullloginProperties: [PassportLoginProperties](../-passport-login-properties/index.md), @NonNulltrackId: [PassportTrackId](../-passport-track-id/index.md)): Intent

Экран запускает авторизацию по trackId. Показывает экран подтверждения входа В onActivityResult возвращается PassportLoginResult если пользователь подтвердил авторизацию.
