//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassport](index.md)/[initialize](initialize.md)

# initialize

[passport]\
fun [initialize](initialize.md)(context: Context, init: [PassportProperties.Builder](../-passport-properties/-builder/index.md).() -&gt; Unit)

Initializing the API for the *:passport* process in Application.onCreate. Be sure to call this method before accessing the library.

&lt;pre&gt; if ([isInPassportProcess()][.isInPassportProcess]) { initializePassport(...) }&lt;/pre&gt;

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportProperties.Builder](../-passport-properties/-builder/index.md) |  |

## Parameters

passport

| | |
|---|---|
| context | application Context |
| init | pasport dsl initializer |
