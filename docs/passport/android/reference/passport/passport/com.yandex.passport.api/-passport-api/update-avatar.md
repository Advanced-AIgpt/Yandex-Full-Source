//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[updateAvatar](update-avatar.md)

# updateAvatar

[passport]\

@WorkerThread

abstract fun [updateAvatar](update-avatar.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNulluri: Uri)

Обновление аватара для аккаунта. Операция может происходить длительное время. Приложение самостоятельно заботится о нужных правах для соответствующих Uri, например, android.permission.READ_EXTERNAL_STORAGE, иначе будет брошено исключение [PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) Такое же исключение может быть брошено и для некорректного Uri.

## Parameters

passport

| | |
|---|---|
| uid | аккаунт для обновления |
| uri | uri аватара |
