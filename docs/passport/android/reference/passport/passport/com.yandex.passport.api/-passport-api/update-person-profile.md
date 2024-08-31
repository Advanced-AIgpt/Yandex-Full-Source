//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[updatePersonProfile](update-person-profile.md)

# updatePersonProfile

[passport]\

@WorkerThread

abstract fun [updatePersonProfile](update-person-profile.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullpersonProfile: [PassportPersonProfile](../-passport-person-profile/index.md))

Обновление пользовательских данных 

 Если какое-то из полей в PersonProfile null, то это поле не будет обновляться.

## Parameters

passport

| | |
|---|---|
| uid | - идентификатор аккаунта для обновления |
| personProfile | - обновленные данные |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportFailedResponseException](../../com.yandex.passport.api.exception/-passport-failed-response-exception/index.md) | message содержит детальную информацию о ошибке. Некоторые из вариантов сообщений об ошибке: firstname.empty firstname.invalid lastname.empty lastname.invalid display_name.invalid |
