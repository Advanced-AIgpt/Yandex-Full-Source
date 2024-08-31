//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportLoginProperties](index.md)/[source](source.md)

# source

[passport]\
abstract val [source](source.md): String?

A certain string by which the source of the authorization can be identified.

If there are two buttons in the application that can trigger authorization, then you need to pass different strings to this method.

This string will be sent to yandex.metrica.
