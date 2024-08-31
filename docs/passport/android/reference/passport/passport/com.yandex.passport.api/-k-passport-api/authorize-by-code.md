//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[authorizeByCode](authorize-by-code.md)

# authorizeByCode

[passport]\
abstract suspend fun [authorizeByCode](authorize-by-code.md)(code: [PassportCode](../-passport-code/index.md)): Result&lt;[PassportAccount](../-passport-account/index.md)&gt;

Возвращает аккаунт [PassportAccount](../-passport-account/index.md) по коду [PassportCode](../-passport-code/index.md).<br></br> Добавляет в базу аккаунтов или обновляет (по uid) существующий аккаунт и его &quot;мастер&quot; токен.<br></br>

#### Return

[PassportAccount](../-passport-account/index.md) аккаунт

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.KPassportApi](get-code.md) |  |

## Parameters

passport

| | |
|---|---|
| code | [PassportCode](../-passport-code/index.md) код подтверждения |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportCodeInvalidException](../../com.yandex.passport.api.exception/-passport-code-invalid-exception/index.md) | неверный или просроченный код подтверждения |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
