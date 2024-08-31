//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[createSocialBindIntent](create-social-bind-intent.md)

# createSocialBindIntent

[passport]\

@CheckResult

@NonNull

@AnyThread

abstract fun [createSocialBindIntent](create-social-bind-intent.md)(@NonNullcontext: Context, @NonNullproperties: [PassportSocialBindProperties](../-passport-social-bind-properties/index.md)): Intent

 Создать интент для связки аккаунта с [PassportUid](../-passport-uid/index.md) uid с социальной сетью из списка [PassportSocialConfiguration](../-passport-social-configuration/index.md) для [isFullSocial](../-passport-social-configuration/is-full-social.md)== true.  Для значений [isFullSocial](../-passport-social-configuration/is-full-social.md)== false будет выброшен IllegalStateException
