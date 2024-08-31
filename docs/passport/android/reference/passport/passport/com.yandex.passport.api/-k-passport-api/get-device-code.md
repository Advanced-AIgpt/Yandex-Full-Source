//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getDeviceCode](get-device-code.md)

# getDeviceCode

[passport]\
abstract suspend fun [getDeviceCode](get-device-code.md)(passportEnvironment: [KPassportEnvironment](../-k-passport-environment/index.md), deviceName: String?, clientBound: Boolean): Result&lt;[PassportDeviceCode](../-passport-device-code/index.md)&gt;

Начинает процесс авторизации другого устройства.

На стороне устройства без авторизации необходимо вызвать данный метод и каким-либо образом (например, qr код) передать userCode на устройство с авторизацией Метод [getDeviceCode](get-device-code.md) вызывает метод [getDeviceCode](get-device-code.md) с параметром <tt>clientBound</tt> равным <tt>true</tt>

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportApi](accept-device-authorization.md) |  |

## Parameters

passport

| | |
|---|---|
| passportEnvironment | -     окружение в котором будет производиться авторизация |
| deviceName | -     опциональное имя устройства |
| clientBound | -     true / false |
