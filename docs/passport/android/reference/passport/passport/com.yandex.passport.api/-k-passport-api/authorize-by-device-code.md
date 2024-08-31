//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[authorizeByDeviceCode](authorize-by-device-code.md)

# authorizeByDeviceCode

[passport]\
abstract suspend fun [authorizeByDeviceCode](authorize-by-device-code.md)(passportEnvironment: [KPassportEnvironment](../-k-passport-environment/index.md), deviceCode: String): Result&lt;[PassportAccount](../-passport-account/index.md)&gt;

Заканчивает авторизацию на устройстве.

Данный метод необходимо вызывать каждые [PassportDeviceCode.getInterval](../-passport-device-code/get-interval.md) на устройстве без авторизации не дольше [PassportDeviceCode.getExpiresIn](../-passport-device-code/get-expires-in.md).

В случае успешной авторизации будет возвращен аккаунт, добавленный на устройство. В случае, если авторизация еще не подтверждена, то будет выброшено PassportAuthorizationPendingException

## Parameters

passport

| | |
|---|---|
| passportEnvironment | -     окружение, в котором будет производиться авторизация |
| deviceCode | -     значение deviceCode, возвращаенное из метода .getDeviceCode |
