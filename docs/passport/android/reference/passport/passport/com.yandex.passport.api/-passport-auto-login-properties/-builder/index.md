//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportAutoLoginProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportAutoLoginProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportAutoLoginProperties](../index.md) |
| [setFilter](set-filter.md) | [passport]<br>abstract fun [setFilter](set-filter.md)(filter: [PassportFilter](../../-passport-filter/index.md)): [PassportAutoLoginProperties.Builder](index.md) |
| [setMessage](set-message.md) | [passport]<br>abstract fun [setMessage](set-message.md)(message: String?): [PassportAutoLoginProperties.Builder](index.md) |
| [setMode](set-mode.md) | [passport]<br>abstract fun [setMode](set-mode.md)(mode: [PassportAutoLoginMode](../../-passport-auto-login-mode/index.md)): [PassportAutoLoginProperties.Builder](index.md) |
| [setTheme](set-theme.md) | [passport]<br>abstract fun [setTheme](set-theme.md)(theme: [PassportTheme](../../-passport-theme/index.md)): [PassportAutoLoginProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [filter](filter.md) | [passport]<br>abstract override var [filter](filter.md): [PassportFilter](../../-passport-filter/index.md) |
| [message](message.md) | [passport]<br>abstract override var [message](message.md): String? |
| [mode](mode.md) | [passport]<br>abstract override var [mode](mode.md): [PassportAutoLoginMode](../../-passport-auto-login-mode/index.md) |
| [theme](theme.md) | [passport]<br>abstract override var [theme](theme.md): [PassportTheme](../../-passport-theme/index.md) |
