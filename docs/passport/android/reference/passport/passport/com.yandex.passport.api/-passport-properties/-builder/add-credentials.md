//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportProperties](../index.md)/[Builder](index.md)/[addCredentials](add-credentials.md)

# addCredentials

[passport]\
abstract fun [addCredentials](add-credentials.md)(environment: [KPassportEnvironment](../../-k-passport-environment/index.md), encryptedId: String, encryptedSecret: String)

[passport]\
abstract fun [addCredentials](add-credentials.md)(environment: [PassportEnvironment](../../-passport-environment/index.md), credentials: [PassportCredentials](../../-passport-credentials/index.md)): [PassportProperties.Builder](index.md)

Добавить зашифрованную пару client id/secret [credentials](../../-passport-credentials/index.md) для окружения [environment](../../-passport-environment/index.md).<br></br>

#### Return

[Builder](index.md)

## Parameters

passport

| | |
|---|---|
| environment | [PassportEnvironment](../../-passport-environment/index.md) окружение |
| credentials | [PassportCredentials](../../-passport-credentials/index.md) зашифрованные client id и client secret |
