//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportLogger](index.md)

# PassportLogger

[passport]\
interface [PassportLogger](index.md) : LoggingDelegate

## Types

| Name | Summary |
|---|---|
| [AndroidLogger](-android-logger/index.md) | [passport]<br>object [AndroidLogger](-android-logger/index.md) : [PassportLogger](index.md) |
| [BlankLogger](-blank-logger/index.md) | [passport]<br>object [BlankLogger](-blank-logger/index.md) : [PassportLogger](index.md), LoggingDelegate |

## Functions

| Name | Summary |
|---|---|
| [log](log.md) | [passport]<br>open override fun [log](log.md)(logLevel: [PassportLogLevel](../index.md#-33048098%2FClasslikes%2F339143592), tag: String, message: String)<br>~~abstract~~ ~~fun~~ [~~log~~](log.md)~~(~~logLevel: Int, tag: String, message: String~~)~~<br>open override fun [log](log.md)(logLevel: [PassportLogLevel](../index.md#-33048098%2FClasslikes%2F339143592), tag: String, message: String, th: Throwable)<br>~~abstract~~ ~~fun~~ [~~log~~](log.md)~~(~~logLevel: Int, tag: String, message: String, th: Throwable~~)~~ |

## Inheritors

| Name |
|---|
| [AndroidLogger](-android-logger/index.md) |
| [BlankLogger](-blank-logger/index.md) |
