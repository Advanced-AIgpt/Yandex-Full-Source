//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[stashValue](stash-value.md)

# stashValue

[passport]\
abstract suspend fun [stashValue](stash-value.md)(uid: [PassportUid](../-passport-uid/index.md), cell: [KPassportStashCell](../-k-passport-stash-cell/index.md), value: String?): Result&lt;Unit&gt;

Добавляет или обновляет сохранённое значение <tt>value</tt> по ключу [cell](../-passport-stash-cell/index.md) для аккаунта [uid](../-passport-uid/index.md).<br></br> Для добавления новых ключей в список [PassportStashCell](../-passport-stash-cell/index.md) свяжитесь с командой разработки этой библиотеки.<br></br>

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportAccount](../-passport-account/get-stash.md) |  |
| [com.yandex.passport.api.PassportStash](../../../passport/com.yandex.passport.api/-passport-stash/get-value.md) |  |

## Parameters

passport

| | |
|---|---|
| uid | [PassportUid](../-passport-uid/index.md) уникальный идентификатор аккаунта |
| cell | [KPassportStashCell](../-k-passport-stash-cell/index.md) ключ |
| value | значение |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\
abstract suspend fun [stashValue](stash-value.md)(passportUids: List&lt;[PassportUid](../-passport-uid/index.md)&gt;, cell: [KPassportStashCell](../-k-passport-stash-cell/index.md), value: String?): Result&lt;Unit&gt;

Функция аналогична .stashValue, но позволяет выполнить операцию сразу для нескольких аккаунтов

## See also

passport

| | |
|---|---|
|  | .stashValue |
