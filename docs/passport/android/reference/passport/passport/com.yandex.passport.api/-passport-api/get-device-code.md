//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getDeviceCode](get-device-code.md)

# getDeviceCode

[passport]\

@Deprecated

@WorkerThread

@NonNull

~~abstract~~ ~~fun~~ [~~getDeviceCode~~](get-device-code.md)~~(~~@NonNullenvironment: [PassportEnvironment](../-passport-environment/index.md), @NullabledeviceName: String~~)~~~~:~~ [PassportDeviceCode](../-passport-device-code/index.md)

Начинает процесс авторизации устройства  На стороне устройства без авторизации необходимо вызвать данный метод и каким-либо образом (например, qr код) передать userCode на устройство с авторизацией  Deprecated: используйте метод [getDeviceCode](get-device-code.md) с параметром clientBound равным true

## See also

passport

| | |
|---|---|
| [getDeviceCode(PassportEnvironment,String,boolean)](get-device-code.md) |  |
| [authorizeByDeviceCode](authorize-by-device-code.md) |  |
| [acceptDeviceAuthorization](accept-device-authorization.md) |  |

## Parameters

passport

| | |
|---|---|
| environment | - окружение в котором будет производиться авторизация |
| deviceName | - опциональное имя устройства |

[passport]\

@CheckResult

@WorkerThread

@NonNull

abstract fun [getDeviceCode](get-device-code.md)(@NonNullenvironment: [PassportEnvironment](../-passport-environment/index.md), @NullabledeviceName: String, clientBound: Boolean): [PassportDeviceCode](../-passport-device-code/index.md)

Начинает процесс авторизации другого устройства  На стороне устройства без авторизации необходимо вызвать данный метод и каким-либо образом (например, qr код) передать userCode на устройство с авторизацией  Метод [getDeviceCode](get-device-code.md) вызывает метод [getDeviceCode](get-device-code.md) с параметром clientBound равным true

## See also

passport

| | |
|---|---|
| [getDeviceCode(com.yandex.passport.api.PassportEnvironment, java.lang.String)](get-device-code.md) |  |
| [authorizeByDeviceCode](authorize-by-device-code.md) |  |
| [acceptDeviceAuthorization](accept-device-authorization.md) |  |

## Parameters

passport

| | |
|---|---|
| environment | - окружение в котором будет производиться авторизация |
| deviceName | - опциональное имя устройства |
| clientBound | - true / false |
