//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[Passport](index.md)/[createPassportCredentials](create-passport-credentials.md)

# createPassportCredentials

[passport]\

@NonNull

@Deprecated

~~open~~ ~~fun~~ [~~createPassportCredentials~~](create-passport-credentials.md)~~(~~@NonNullencryptedId: String, @NonNullencryptedSecret: String~~)~~~~:~~ [PassportCredentials](../-passport-credentials/index.md)

Создаёт контейнер для зашифрованной пары client id / client secret. Подробнее смотрите в документации [по регистрации oauth приложения.](https://wiki.yandex-team.ru/yandexmobile/accountmanager/#passportparameters)

#### Return

[PassportCredentials](../-passport-credentials/index.md)

#### Deprecated

use [PassportCredentials.Factory](../-passport-credentials/-factory/index.md) for java clients

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCredentials.Factory](../../../passport/com.yandex.passport.api/-passport-credentials/-factory/from.md) |  |

## Parameters

passport

| | |
|---|---|
| encryptedId | зашифрованный client id |
| encryptedSecret | зашифрованный client secret |
