//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAuthorizationUrlProperties](index.md)

# PassportAuthorizationUrlProperties

[passport]\
interface [PassportAuthorizationUrlProperties](index.md)

## Types

| Name | Summary |
|---|---|
| [Builder](-builder/index.md) | [passport]<br>interface [Builder](-builder/index.md) : [PassportAuthorizationUrlProperties](index.md) |
| [Companion](-companion/index.md) | [passport]<br>object [Companion](-companion/index.md) |

## Properties

| Name | Summary |
|---|---|
| [analyticsParams](analytics-params.md) | [passport]<br>abstract val [analyticsParams](analytics-params.md): Map&lt;String, String&gt;<br>Параметры, которые будут отправлены в метрику вместе с событием core.get_auth_url <br></br> Возможные значения ключей: <br></br> |
| [returnUrl](return-url.md) | [passport]<br>abstract val [returnUrl](return-url.md): String<br>URL на который произойдёт переход после успешной авторизации. Обязательное значение. |
| [tld](tld.md) | [passport]<br>abstract val [tld](tld.md): String<br>TLD, где происходит авторизация: *ru*, *com*, *com.tr*, *ua* и т.д. Обязательное значение. |
| [uid](uid.md) | [passport]<br>abstract val [uid](uid.md): [PassportUid](../-passport-uid/index.md)<br>[PassportUid](../-passport-uid/index.md). Обязательное значение. |

## Inheritors

| Name |
|---|
| [Builder](-builder/index.md) |
