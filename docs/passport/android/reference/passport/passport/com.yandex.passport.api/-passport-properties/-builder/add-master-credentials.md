//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportProperties](../index.md)/[Builder](index.md)/[addMasterCredentials](add-master-credentials.md)

# addMasterCredentials

[passport]\
abstract fun [addMasterCredentials](add-master-credentials.md)(environment: [KPassportEnvironment](../../-k-passport-environment/index.md), encryptedId: String, encryptedSecret: String)

[passport]\
abstract fun [addMasterCredentials](add-master-credentials.md)(environment: [PassportEnvironment](../../-passport-environment/index.md), credentials: [PassportCredentials](../../-passport-credentials/index.md)): [PassportProperties.Builder](index.md)

Метод не должен использоваться приложениями без согласования с командой паспорта.

#### Return

[Builder](index.md)

## Parameters

passport

| | |
|---|---|
| environment | [PassportEnvironment](../../-passport-environment/index.md) окружение |
| credentials | [PassportCredentials](../../-passport-credentials/index.md) зашифрованные client id и client secret |
