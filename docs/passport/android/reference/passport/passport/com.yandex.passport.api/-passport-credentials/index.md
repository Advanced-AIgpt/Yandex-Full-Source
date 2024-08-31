//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportCredentials](index.md)

# PassportCredentials

[passport]\
interface [PassportCredentials](index.md)

Содержит зашифрованную пару client id / client secret.<br></br><br></br> Подробнее смотрите в документации [по регистрации oauth приложения.](https://wiki.yandex-team.ru/yandexmobile/accountmanager/#passportparameters)<br></br>

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Properties

| Name | Summary |
|---|---|
| [encryptedId](encrypted-id.md) | [passport]<br>abstract val [encryptedId](encrypted-id.md): String<br>Возвращает зашифрованный client id |
| [encryptedSecret](encrypted-secret.md) | [passport]<br>abstract val [encryptedSecret](encrypted-secret.md): String<br>Возвращает зашифрованный client secret |
