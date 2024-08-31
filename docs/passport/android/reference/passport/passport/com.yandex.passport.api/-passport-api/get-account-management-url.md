//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getAccountManagementUrl](get-account-management-url.md)

# getAccountManagementUrl

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [getAccountManagementUrl](get-account-management-url.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md)): Uri

Возвращает url страницы на которой пользователь может просмотреть или изменить свои данные в аккаунте. После того как пользователь закрыл эту страницу необходимо обновить пользовательские данные: [performSync](perform-sync.md)

## Parameters

passport

| | |
|---|---|
| uid | - uid аккаунта |
