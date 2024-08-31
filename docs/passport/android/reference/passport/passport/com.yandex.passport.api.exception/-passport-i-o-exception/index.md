//[passport](../../../index.md)/[com.yandex.passport.api.exception](../index.md)/[PassportIOException](index.md)

# PassportIOException

[passport]\
open class [PassportIOException](index.md) : IOException

Содержит IOException из процесса *:passport*. Обычно появление этого эксепшена означает, что произошла сетевая ошибка и действие можно будет повторить после появления сети. В зависимости от логики приложения, может быть удобно ловить его не отдельно, а вместе с остальными IOException.

## Constructors

| | |
|---|---|
| [PassportIOException](-passport-i-o-exception.md) | [passport]<br>open fun [PassportIOException](-passport-i-o-exception.md)(@NonNullcause: Throwable) |
