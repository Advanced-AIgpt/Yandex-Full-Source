//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportTurboAppResult](index.md)

# PassportTurboAppResult

[passport]\
class [PassportTurboAppResult](index.md)(val token: [PassportToken](../-passport-token/index.md), val jwtToken: [PassportTurboAppJwtToken](../-passport-turbo-app-jwt-token/index.md), val flowErrorCodes: List&lt;String&gt;, val grantedScopes: List&lt;String&gt;)

## Constructors

| | |
|---|---|
| [PassportTurboAppResult](-passport-turbo-app-result.md) | [passport]<br>fun [PassportTurboAppResult](-passport-turbo-app-result.md)(token: [PassportToken](../-passport-token/index.md), jwtToken: [PassportTurboAppJwtToken](../-passport-turbo-app-jwt-token/index.md), flowErrorCodes: List&lt;String&gt;, grantedScopes: List&lt;String&gt;) |

## Properties

| Name | Summary |
|---|---|
| [flowErrorCodes](flow-error-codes.md) | [passport]<br>val [flowErrorCodes](flow-error-codes.md): List&lt;String&gt;<br>Коды ошибок, которые возникали в процессе авторизации. Несмотря на подобные ошибки авторизация прошла успешно. |
| [grantedScopes](granted-scopes.md) | [passport]<br>val [grantedScopes](granted-scopes.md): List&lt;String&gt;<br>Список скоупов, которые выдал пользователь в данном токене. |
| [jwtToken](jwt-token.md) | [passport]<br>val [jwtToken](jwt-token.md): [PassportTurboAppJwtToken](../-passport-turbo-app-jwt-token/index.md) |
| [token](token.md) | [passport]<br>val [token](token.md): [PassportToken](../-passport-token/index.md) |
