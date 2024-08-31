//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportCode](index.md)

# PassportCode

[passport]\
interface [PassportCode](index.md)

Содержит код (&quot;код подтверждения&quot;), полученный от метода [PassportApi.getCode](../../../../passport/passport/com.yandex.passport.api/-passport-api/get-code.md) или [Factory.from](-factory/from.md).<br></br><br></br> Подробнее см. описание метода [PassportApi.getCode](../../../../passport/passport/com.yandex.passport.api/-passport-api/get-code.md).<br></br><br></br> Пример значения: <tt>3ujb65647gy6pklk</tt><br></br>

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Properties

| Name | Summary |
|---|---|
| [environment](environment.md) | [passport]<br>abstract val [environment](environment.md): [PassportEnvironment](../-passport-environment/index.md) |
| [expiresIn](expires-in.md) | [passport]<br>abstract val [expiresIn](expires-in.md): Int<br>Значение в секундах - через сколько данный токен будет не валиден |
| [value](value.md) | [passport]<br>abstract val [value](value.md): String |
