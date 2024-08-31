//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[dropToken](drop-token.md)

# dropToken

[passport]\
abstract suspend fun [dropToken](drop-token.md)(token: String): Result&lt;Unit&gt;

Удалить токен по его значению.<br></br> Токен будет удалён из внутренней таблицы. Если токен уже удалён, никаких действий производиться не будет.<br></br><br></br> В случае, если сервер ответил, что токен больше не валиден, нужно его удалить, вызвав этот метод до запроса нового токена [getToken](get-token.md)<br></br>

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportApi](get-token.md) |  |

## Parameters

passport

| | |
|---|---|
| token | значение токена |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
