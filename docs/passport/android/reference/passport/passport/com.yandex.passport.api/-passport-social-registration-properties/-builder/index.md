//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportSocialRegistrationProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportSocialRegistrationProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportSocialRegistrationProperties](../index.md) |
| [setMessage](set-message.md) | [passport]<br>abstract fun [setMessage](set-message.md)(message: String): [PassportSocialRegistrationProperties.Builder](index.md)<br>Сообщение для пользователя, показываемое в начале социальной регистрации. Может содержать информацию, какие преимущества получит пользователь от дорегистрации. |
| [setUid](set-uid.md) | [passport]<br>abstract fun [setUid](set-uid.md)(uid: [PassportUid](../../-passport-uid/index.md)): [PassportSocialRegistrationProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [message](message.md) | [passport]<br>abstract override var [message](message.md): String? |
| [uid](uid.md) | [passport]<br>abstract override var [uid](uid.md): [PassportUid](../../-passport-uid/index.md)? |
