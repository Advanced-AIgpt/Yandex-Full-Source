//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[acceptDeviceAuthorization](accept-device-authorization.md)

# acceptDeviceAuthorization

[passport]\
abstract suspend fun [acceptDeviceAuthorization](accept-device-authorization.md)(uid: [PassportUid](../-passport-uid/index.md), userCode: String): Result&lt;Unit&gt;

Подтверждает авторизацию на устройстве

После получения  userCode из метода .getDeviceCode необходимо его передать в этот метод.

## Parameters

passport

| | |
|---|---|
| uid | -     аккаунт, в котором необходимо авторизоваться. |
| userCode | -     код из метода .getDeviceCode |
