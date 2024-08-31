//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportApi](index.md)/[getAuthorizationUrl](get-authorization-url.md)

# getAuthorizationUrl

[passport]\

@WorkerThread

@CheckResult

@NonNull

abstract fun [getAuthorizationUrl](get-authorization-url.md)(@NonNullproperties: [PassportAuthorizationUrlProperties](../-passport-authorization-url-properties/index.md)): String

Возвращает URL для авторизации внутри WebView, то есть меняет авторизацию в приложении на авторизацию по куке.yandexuidCookieValue значение куки [yandexuid](https://wiki.yandex-team.ru/cookies/yandexuid/) Используется для защиты от CSRF атак. Кука проставляется на любом сервере Яндекса в соответствующем TLD до вызова авторизации. Возможно, в данный момент времени эта защита не включена, но может быть включена позднее. В этом случае, без равенства yandexuid в урле и куке пользователя дополнительно попросят подтвердить вход. Q. Можно ли этот метод использовать для авторизации на любой странице Яндекса? A. Можно, главное, чтобы значение параметра tld и TLD внутри returnUrl совпадали. Q. Что будет, если передать URL на произвольную страницу? A. Бэкенд проверяет returnUrl и может отказаться генерировать ссылку.

#### Return

URL для авторизации

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportAuthorizationUrlProperties](../-passport-authorization-url-properties/index.md) |  |

## Parameters

passport

| | |
|---|---|
| properties | - параметры ссылки |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | аккаунт с таким uid найден, но валидный токен отсутствует |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |

[passport]\

@Deprecated

@WorkerThread

@CheckResult

@NonNull

~~abstract~~ ~~fun~~ [~~getAuthorizationUrl~~](get-authorization-url.md)~~(~~@NonNulluid: [PassportUid](../-passport-uid/index.md), @NonNullreturnUrl: String, @NonNulltld: String, @NullableyandexuidCookieValue: String~~)~~~~:~~ String

#### Deprecated

Используйте [getAuthorizationUrl](get-authorization-url.md)
