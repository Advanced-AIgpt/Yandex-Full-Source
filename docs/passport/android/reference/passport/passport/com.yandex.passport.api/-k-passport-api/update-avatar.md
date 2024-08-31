//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[updateAvatar](update-avatar.md)

# updateAvatar

[passport]\
abstract suspend fun [updateAvatar](update-avatar.md)(uid: [PassportUid](../-passport-uid/index.md), uri: Uri): Result&lt;Unit&gt;

Обновление аватара для аккаунта. Операция может происходить длительное время.<br></br><br></br> Приложение самостоятельно заботится о нужных правах для соответствующих <tt>Uri</tt>,<br></br> например, <tt>android.permission.READ_EXTERNAL_STORAGE</tt>, иначе будет брошено исключение [PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md)<br></br> Такое же исключение может быть брошено и для некорректного <tt>Uri</tt>.<br></br>

## Parameters

passport

| | |
|---|---|
| uid | аккаунт для обновления |
| uri | uri аватара |
