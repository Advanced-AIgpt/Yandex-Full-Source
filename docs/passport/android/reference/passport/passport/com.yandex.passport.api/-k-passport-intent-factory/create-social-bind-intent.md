//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportIntentFactory](index.md)/[createSocialBindIntent](create-social-bind-intent.md)

# createSocialBindIntent

[passport]\
abstract fun [createSocialBindIntent](create-social-bind-intent.md)(context: Context, properties: [PassportSocialBindProperties](../-passport-social-bind-properties/index.md)): Intent

Создать интент для связки аккаунта с [PassportUid](../-passport-uid/index.md) uid с социальной сетью из списка [PassportSocialConfiguration](../-passport-social-configuration/index.md) для [PassportSocialConfiguration.isFullSocial](../-passport-social-configuration/is-full-social.md)<tt>== true</tt>.

Для значений [PassportSocialConfiguration.isFullSocial](../-passport-social-configuration/is-full-social.md)<tt>== false</tt> будет выброшен IllegalStateException
