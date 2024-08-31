//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[performLinkageForce](perform-linkage-force.md)

# performLinkageForce

[passport]\
abstract suspend fun [performLinkageForce](perform-linkage-force.md)(firstUid: [PassportUid](../-passport-uid/index.md), secondUid: [PassportUid](../-passport-uid/index.md)): Result&lt;Unit&gt;

Выполняет связку аккаунтов без выполнения проверок на стороне АМ.

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | -     содержит опциональный uid аккаунта в который необходимо выполнить релогин. |
| [com.yandex.passport.api.exception.PassportFailedResponseException](../../com.yandex.passport.api.exception/-passport-failed-response-exception/index.md) | -     содержит message с ответом сервера об ошибке. Например: &quot;profile.not_allowed&quot; |
