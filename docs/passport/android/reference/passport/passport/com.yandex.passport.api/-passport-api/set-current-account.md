//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[setCurrentAccount](set-current-account.md)

# setCurrentAccount

[passport]\

@WorkerThread

abstract fun [setCurrentAccount](set-current-account.md)(@NonNulluid: [PassportUid](../-passport-uid/index.md))

Сохраняет UID и имя аккаунта с переданным [PassportUid](../-passport-uid/index.md) в SharedPreferences вызывающего приложения. Впоследствии его можно извлечь использованием функции [getCurrentAccount](get-current-account.md). Стоит заметить, что при невозможности обнаружить аккаунт с переданным Uid будет выброшено исключение.

#### Return

[PassportAccount](../-passport-account/index.md) который требуется сохранить

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportApi](get-current-account.md) |  |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) |  |
