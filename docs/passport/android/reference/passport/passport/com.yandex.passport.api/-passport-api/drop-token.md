//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[dropToken](drop-token.md)

# dropToken

[passport]\

@WorkerThread

abstract fun [dropToken](drop-token.md)(@NonNulltoken: String)

Удалить токен по его значению. Токен будет удалён из внутренней таблицы. Если токен уже удалён, никаких действий производиться не будет. В случае, если сервер ответил, что токен больше не валиден, нужно его удалить, вызвав этот метод до запроса нового токена getToken

## See also

passport

| | |
|---|---|
| com.yandex.passport.api.PassportApi |  |

## Parameters

passport

| | |
|---|---|
| token | значение токена |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
