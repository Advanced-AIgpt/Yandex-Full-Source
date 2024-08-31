//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getCurrentAccount](get-current-account.md)

# getCurrentAccount

[passport]\
abstract suspend fun [getCurrentAccount](get-current-account.md)(): Result&lt;[PassportAccount](../-passport-account/index.md)?&gt;

Возвращает системный аккаунт, который был ранее сохранен через вызов [setCurrentAccount](set-current-account.md). Информация о сохраненном аккаунте записывается в SharedPreferences того приложения, которое осуществляет вызов вышеуказанной функции.

#### Return

&quot;текущий&quot; аккаунт [PassportAccount](../-passport-account/index.md)

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportApi](set-current-account.md) |  |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) |  |
