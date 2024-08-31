//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getAccounts](get-accounts.md)

# getAccounts

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [getAccounts](get-accounts.md)(@NonNullfilter: [PassportFilter](../-passport-filter/index.md)): List&lt;[PassportAccount](../-passport-account/index.md)&gt;

Возвращает список аккаунтов, соответствующих фильтру [PassportFilter](../-passport-filter/index.md). &quot;Тимовские&quot; аккаунты [PASSPORT_ENVIRONMENT_TEAM_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-p-r-o-d-u-c-t-i-o-n.md) и [PASSPORT_ENVIRONMENT_TEAM_TESTING](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-t-e-s-t-i-n-g.md) не различаются между собой при добавлении в список.

#### Return

список аккаунтов [PassportAccount](../-passport-account/index.md)

## Parameters

passport

| | |
|---|---|
| filter | [PassportFilter](../-passport-filter/index.md) фильтр для списка |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
