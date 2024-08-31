//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[acceptDeviceAuthorization](accept-device-authorization.md)

# acceptDeviceAuthorization

[passport]\

@WorkerThread

abstract fun [acceptDeviceAuthorization](accept-device-authorization.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNulluserCode: String)

Подтверждает авторизацию на устройстве После получения userCode из метода getDeviceCode необходимо его передать в этот метод.

## Parameters

passport

| | |
|---|---|
| uid | - аккаунт, в котором необходимо авторизоваться. |
| userCode | - код из метода getDeviceCode |
