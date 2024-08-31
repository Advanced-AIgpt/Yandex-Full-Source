//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[performLinkageForce](perform-linkage-force.md)

# performLinkageForce

[passport]\

@WorkerThread

abstract fun [performLinkageForce](perform-linkage-force.md)(@NonNullfirstUid: [PassportUid](../-passport-uid/index.md), @NonNullsecondUid: [PassportUid](../-passport-uid/index.md))

Выполняет связку аккаунтов без выполнения проверок на стороне АМ.

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | - содержит опциональный uid аккаунта в который необходимо выполнить релогин. |
| [com.yandex.passport.api.exception.PassportFailedResponseException](../../com.yandex.passport.api.exception/-passport-failed-response-exception/index.md) | - содержит message с ответом сервера об ошибке. Например: &quot;profile.not_allowed&quot; |
