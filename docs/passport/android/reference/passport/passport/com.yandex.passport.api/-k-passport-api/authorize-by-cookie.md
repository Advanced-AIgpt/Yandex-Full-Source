//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[authorizeByCookie](authorize-by-cookie.md)

# authorizeByCookie

[passport]\
abstract suspend fun [authorizeByCookie](authorize-by-cookie.md)(cookie: [PassportCookie](../-passport-cookie/index.md)): Result&lt;[PassportAccount](../-passport-account/index.md)&gt;

Возвращает аккаунт [PassportAccount](../-passport-account/index.md) по cookie [PassportCookie](../-passport-cookie/index.md), полученной из результата работы WebView или из каких-либо внешних источников.<br></br> Добавляет в базу аккаунтов или обновляет (по uid) существующий аккаунт и его &quot;мастер&quot; токен.<br></br>

#### Return

[PassportAccount](../-passport-account/index.md) аккаунт

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCookie.Factory](../-passport-cookie/-factory/from-all-cookies.md) |  |

## Parameters

passport

| | |
|---|---|
| cookie | [PassportCookie](../-passport-cookie/index.md) куки, по которым производится авторизация |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportCookieInvalidException](../../com.yandex.passport.api.exception/-passport-cookie-invalid-exception/index.md) | неверные или просроченные cookie |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
