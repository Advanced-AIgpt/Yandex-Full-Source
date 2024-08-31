//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportCookie](../index.md)/[Factory](index.md)/[fromAllCookies](from-all-cookies.md)

# fromAllCookies

[passport]\
fun [fromAllCookies](from-all-cookies.md)(environment: [PassportEnvironment](../../-passport-environment/index.md), cookies: String?, returnUrl: String): [PassportCookie](../index.md)

## Parameters

passport

| | |
|---|---|
| returnUrl | url ресурса, для которого были выданы авторизационные куки. Если будет передан некорректный url, будет выброшено исключение |
| cookies | строка вида &quot;key1=value1; key2=value2; …&quot;, содержащая все куки (сейчас их около 20) |
