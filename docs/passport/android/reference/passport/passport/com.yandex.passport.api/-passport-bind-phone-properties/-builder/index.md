//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportBindPhoneProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportBindPhoneProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportBindPhoneProperties](../index.md) |
| [disablePhoneEditability](disable-phone-editability.md) | [passport]<br>abstract fun [disablePhoneEditability](disable-phone-editability.md)(): [PassportBindPhoneProperties.Builder](index.md) |
| [setPhoneNumber](set-phone-number.md) | [passport]<br>abstract fun [setPhoneNumber](set-phone-number.md)(phoneNumber: String?): [PassportBindPhoneProperties.Builder](index.md) |
| [setTheme](set-theme.md) | [passport]<br>abstract fun [setTheme](set-theme.md)(theme: [PassportTheme](../../-passport-theme/index.md)): [PassportBindPhoneProperties.Builder](index.md) |
| [setUid](set-uid.md) | [passport]<br>abstract fun [setUid](set-uid.md)(uid: [PassportUid](../../-passport-uid/index.md)): [PassportBindPhoneProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [isPhoneEditable](is-phone-editable.md) | [passport]<br>abstract override var [isPhoneEditable](is-phone-editable.md): Boolean<br>Если передан номер телефона, то по умолчанию он подставляется и пользователь его может отредактировать. Данный флаг запрещает редактирование номера. Пользователь может нажать только &quot;Далее&quot;. |
| [phoneNumber](phone-number.md) | [passport]<br>abstract override var [phoneNumber](phone-number.md): String?<br>Опцональный параметр. Если не передан, то отобразится пустая форма ввода номера и запросим номер из гуглосервисов. phoneNumber номер телефона для привязки |
| [theme](theme.md) | [passport]<br>abstract override var [theme](theme.md): [PassportTheme](../../-passport-theme/index.md)<br>Выбор общей темы из [PassportTheme](../../-passport-theme/index.md) |
| [uid](uid.md) | [passport]<br>abstract override var [uid](uid.md): [PassportUid](../../-passport-uid/index.md)<br>Обязательный параметр [PassportUid](../../-passport-uid/index.md) аккаунта к которому необходимо привязать номер |
