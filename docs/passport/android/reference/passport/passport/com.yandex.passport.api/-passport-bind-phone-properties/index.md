//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportBindPhoneProperties](index.md)

# PassportBindPhoneProperties

[passport]\
interface [PassportBindPhoneProperties](index.md)

## Types

| Name | Summary |
|---|---|
| [Builder](-builder/index.md) | [passport]<br>interface [Builder](-builder/index.md) : [PassportBindPhoneProperties](index.md) |

## Properties

| Name | Summary |
|---|---|
| [isPhoneEditable](is-phone-editable.md) | [passport]<br>abstract val [isPhoneEditable](is-phone-editable.md): Boolean<br>Если передан номер телефона, то по умолчанию он подставляется и пользователь его может отредактировать. Данный флаг запрещает редактирование номера. Пользователь может нажать только &quot;Далее&quot;. |
| [phoneNumber](phone-number.md) | [passport]<br>abstract val [phoneNumber](phone-number.md): String?<br>Опцональный параметр. Если не передан, то отобразится пустая форма ввода номера и запросим номер из гуглосервисов. phoneNumber номер телефона для привязки |
| [theme](theme.md) | [passport]<br>abstract val [theme](theme.md): [PassportTheme](../-passport-theme/index.md)<br>Выбор общей темы из [PassportTheme](../-passport-theme/index.md) |
| [uid](uid.md) | [passport]<br>abstract val [uid](uid.md): [PassportUid](../-passport-uid/index.md)<br>Обязательный параметр [PassportUid](../-passport-uid/index.md) аккаунта к которому необходимо привязать номер |

## Inheritors

| Name |
|---|
| [Builder](-builder/index.md) |
