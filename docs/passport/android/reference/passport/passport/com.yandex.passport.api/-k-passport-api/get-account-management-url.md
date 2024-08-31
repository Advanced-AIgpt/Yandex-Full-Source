//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getAccountManagementUrl](get-account-management-url.md)

# getAccountManagementUrl

[passport]\
abstract suspend fun [getAccountManagementUrl](get-account-management-url.md)(uid: [PassportUid](../-passport-uid/index.md)): Result&lt;Uri&gt;

Возвращает url страницы на которой пользователь может просмотреть или изменить свои данные в аккаунте.

После того как пользователь закрыл эту страницу необходимо обновить пользовательские данные: [performSync](perform-sync.md)

*Exceptions:*

PassportIOException::class, PassportFailedResponseException::class, PassportAccountNotFoundException::class, PassportRuntimeUnknownException::class

## Parameters

passport

| | |
|---|---|
| uid | -     uid аккаунта |
