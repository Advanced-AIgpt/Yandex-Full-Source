//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportApi](index.md)/[addAccount](add-account.md)

# addAccount

[passport]\
abstract suspend fun [addAccount](add-account.md)(passportEnvironment: [KPassportEnvironment](../-k-passport-environment/index.md), masterToken: String): Result&lt;[PassportAccount](../-passport-account/index.md)?&gt;

Добавить почтовый аккаунт внешних почт (&quot;mailish&quot;)<br></br> Этим методом можно добавить только такой тип аккаунтов<br></br>

#### Return

возвращает [uid](../-passport-uid/index.md) аккаунта, в который нужно дополнительно добавить параметры [CELL_GIMAP_TRACK](../-passport-stash-cell/-c-e-l-l_-g-i-m-a-p_-t-r-a-c-k.md) и [CELL_MAILISH_SOCIAL_CODE](../-passport-stash-cell/-c-e-l-l_-m-a-i-l-i-s-h_-s-o-c-i-a-l_-c-o-d-e.md) с помощью метода .stashValue

## See also

passport

| | |
|---|---|
|  | .getAccount |

## Parameters

passport

| | |
|---|---|
| passportEnvironment | -     [KPassportEnvironment](../-k-passport-environment/index.md) |
| masterToken | -     строка с мастер-токеном |

## Throws

| | |
|---|---|
| [com.yandex.passport.api.exception.PassportIOException](../../com.yandex.passport.api.exception/-passport-i-o-exception/index.md) | ошибка сети, нужно повторить запрос |
| [com.yandex.passport.api.exception.PassportAccountNotAuthorizedException](../../com.yandex.passport.api.exception/-passport-account-not-authorized-exception/index.md) | токен не валидный или не &quot;mailish&quot; |
| [com.yandex.passport.api.exception.PassportFailedResponseException](../../com.yandex.passport.api.exception/-passport-failed-response-exception/index.md) |  |
| [com.yandex.passport.api.exception.PassportRuntimeUnknownException](../../com.yandex.passport.api.exception/-passport-runtime-unknown-exception/index.md) | внутренняя ошибка |
