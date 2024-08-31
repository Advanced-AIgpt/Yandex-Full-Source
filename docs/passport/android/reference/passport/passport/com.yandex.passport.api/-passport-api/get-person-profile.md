//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getPersonProfile](get-person-profile.md)

# getPersonProfile

[passport]\

@CheckResult

@WorkerThread

@NonNull

abstract fun [getPersonProfile](get-person-profile.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), needDisplayNameVariants: Boolean): [PassportPersonProfile](../-passport-person-profile/index.md)

Получение пользовательских данных для дальнейшего их редактирования

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportApi](update-person-profile.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | - идентификатор аккаунта для обновления |
| needDisplayNameVariants | - необходим ли список подсказок для displayName или нет |
