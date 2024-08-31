//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[stashValue](stash-value.md)

# stashValue

[passport]\

@WorkerThread

abstract fun [stashValue](stash-value.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullcell: String, @Nullablevalue: String)

Добавляет или обновляет сохранённое значение value по ключу [cell](../-passport-stash-cell/index.md) для аккаунта [uid](../-passport-uid/index.md). Для добавления новых ключей в список [PassportStashCell](../-passport-stash-cell/index.md) свяжитесь с командой разработки этой библиотеки.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportAccount](../-passport-account/get-stash.md) |  |
| [com.yandex.passport.api.PassportStash](../-passport-stash/get-value.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) уникальный идентификатор аккаунта |
| cell | [PassportStashCell](../-passport-stash-cell/index.md) ключ |
| value | значение |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\

@WorkerThread

abstract fun [stashValue](stash-value.md)(@NonNulluids: List&lt;[PassportUid](../-passport-uid/index.md)&gt;, @NonNullcell: String, @Nullablevalue: String)

Функция аналогична [stashValue](stash-value.md), но позволяет выполнить операцию сразу для нескольких аккаунтов

## See also

passport

| | |
|---|---|
| [stashValue(PassportUid, String, String)](stash-value.md) |  |
