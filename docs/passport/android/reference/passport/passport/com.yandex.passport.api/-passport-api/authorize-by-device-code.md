//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[authorizeByDeviceCode](authorize-by-device-code.md)

# authorizeByDeviceCode

[passport]\

@NonNull

@WorkerThread

@CheckResult

abstract fun [authorizeByDeviceCode](authorize-by-device-code.md)(@NonNullenvironment: [PassportEnvironment](../-passport-environment/index.md), @NonNulldeviceCode: String): [PassportAccount](../-passport-account/index.md)

Заканчивает авторизацию на устройстве. Данный метод необходимо вызывать каждые [getInterval](../-passport-device-code/get-interval.md) на устройстве без авторизации не дольше [getExpiresIn](../-passport-device-code/get-expires-in.md) В случае успешной авторизации будет возвращен аккаунт, добавленный на устройство. В случае, если авторизация еще не подтверждена, то будет выброшено PassportAuthorizationPendingException

## Parameters

passport

| | |
|---|---|
| environment | - окружение, в котором будет производиться авторизация |
| deviceCode | - значение deviceCode, возвращаенное из метода getDeviceCode |
