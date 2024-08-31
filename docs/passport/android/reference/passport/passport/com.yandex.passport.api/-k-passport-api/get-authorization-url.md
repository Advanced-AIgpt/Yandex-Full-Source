//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[getAuthorizationUrl](get-authorization-url.md)

# getAuthorizationUrl

[passport]\
abstract suspend fun [getAuthorizationUrl](get-authorization-url.md)(properties: [PassportAuthorizationUrlProperties.Builder](../-passport-authorization-url-properties/-builder/index.md).() -&gt; Unit): Result&lt;String&gt;

Возвращает URL для авторизации внутри WebView, то есть меняет авторизацию в приложении на авторизацию по куке.<br></br><br></br><u>yandexuidCookieValue</u> значение куки [yandexuid](https://wiki.yandex-team.ru/cookies/yandexuid/)<br></br> Используется для защиты от CSRF атак. Кука проставляется на любом сервере Яндекса в соответствующем TLD до вызова авторизации.<br></br> Возможно, в данный момент времени эта защита не включена, но может быть включена позднее.<br></br> В этом случае, без равенства yandexuid в урле и куке пользователя дополнительно попросят подтвердить вход.<br></br><br></br> Q. Можно ли этот метод использовать для авторизации на любой странице Яндекса?<br></br> A. Можно, главное, чтобы значение параметра <u>tld</u> и TLD внутри <u>returnUrl</u> совпадали.<br></br><br></br> Q. Что будет, если передать URL на произвольную страницу?<br></br> A. Бэкенд проверяет <u>returnUrl</u> и может отказаться генерировать ссылку.<br></br><br></br>

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
| properties | -     параметры ссылки |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportAccountNotFoundException](../../com.yandex.passport.api.exception/-passport-account-not-found-exception/index.md) | аккаунт с таким uid не найден |
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | аккаунт с таким uid найден, но валидный токен отсутствует |
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
