//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[authorizeByCookie](authorize-by-cookie.md)

# authorizeByCookie

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [authorizeByCookie](authorize-by-cookie.md)(@NonNullcookie: [PassportCookie](../-passport-cookie/index.md)): [PassportAccount](../-passport-account/index.md)

Возвращает аккаунт [PassportAccount](../-passport-account/index.md) по cookie [PassportCookie](../-passport-cookie/index.md), полученной из результата работы WebView или из каких-либо внешних источников. Добавляет в базу аккаунтов или обновляет (по uid) существующий аккаунт и его &quot;мастер&quot; токен.

#### Return

[PassportAccount](../-passport-account/index.md) аккаунт

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportCookie.Factory](../../../passport/com.yandex.passport.api/-passport-cookie/-factory/from-all-cookies.md) |  |

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
