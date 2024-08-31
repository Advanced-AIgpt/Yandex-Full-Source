//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getAccount](get-account.md)

# getAccount

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [getAccount](get-account.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md)): [PassportAccount](../-passport-account/index.md)

Возвращает аккаунт [PassportAccount](../-passport-account/index.md) по уникальному идентификатору [PassportUid](../-passport-uid/index.md)

Будет найден аккаунт с нужным uid.В качестве источника аккаунтов используется системная база – результат вызова getAccountsByType(&quot;com.yandex.passport&quot;).В процессе возможно восстановление из внутреннего бекапа и обновление данных в системной базе, в частности при отсутствии uid у одного из аккаунтов он может быть получен из сети по авторизационному токену.

#### Return

аккаунт [PassportAccount](../-passport-account/index.md)

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) для поиска |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\

@Deprecated

@WorkerThread

@CheckResult

@NonNull

~~abstract~~ ~~fun~~ [~~getAccount~~](get-account.md)~~(~~@NonNullaccountName: String~~)~~~~:~~ [PassportAccount](../-passport-account/index.md)

Возвращает аккаунт [PassportAccount](../-passport-account/index.md) по идентификатору name. Вместо этого метода нужно использовать [getAccount](get-account.md), кроме случаев, когда требуется миграция базы приложения, где отсутствовал uid

#### Deprecated

use [getAccount](get-account.md)
