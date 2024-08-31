//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportAuthorizationUrlProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportAuthorizationUrlProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [addAnalyticsParam](add-analytics-param.md) | [passport]<br>abstract fun [addAnalyticsParam](add-analytics-param.md)(key: String, value: String?): [PassportAuthorizationUrlProperties.Builder](index.md) |
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportAuthorizationUrlProperties](../index.md)<br>Требуется указать uid, returnUrl и tld. |
| [setReturnUrl](set-return-url.md) | [passport]<br>abstract fun [setReturnUrl](set-return-url.md)(returnUrl: String): [PassportAuthorizationUrlProperties.Builder](index.md) |
| [setTld](set-tld.md) | [passport]<br>abstract fun [setTld](set-tld.md)(tld: String): [PassportAuthorizationUrlProperties.Builder](index.md) |
| [setUid](set-uid.md) | [passport]<br>abstract fun [setUid](set-uid.md)(uid: [PassportUid](../../-passport-uid/index.md)): [PassportAuthorizationUrlProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [analyticsParams](analytics-params.md) | [passport]<br>abstract override var [analyticsParams](analytics-params.md): MutableMap&lt;String, String&gt;<br>Параметры, которые будут отправлены в метрику вместе с событием core.get_auth_url <br></br> Возможные значения ключей: <br></br> |
| [returnUrl](return-url.md) | [passport]<br>abstract override var [returnUrl](return-url.md): String<br>URL на который произойдёт переход после успешной авторизации. Обязательное значение. |
| [tld](tld.md) | [passport]<br>abstract override var [tld](tld.md): String<br>TLD, где происходит авторизация: *ru*, *com*, *com.tr*, *ua* и т.д. Обязательное значение. |
| [uid](uid.md) | [passport]<br>abstract override var [uid](uid.md): [PassportUid](../../-passport-uid/index.md)<br>[PassportUid](../../-passport-uid/index.md). Обязательное значение. |
