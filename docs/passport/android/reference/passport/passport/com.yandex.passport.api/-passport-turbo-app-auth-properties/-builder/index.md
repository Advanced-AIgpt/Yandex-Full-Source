//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportTurboAppAuthProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>open class [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportTurboAppAuthProperties](../index.md) |
| [setClientId](set-client-id.md) | [passport]<br>abstract fun [setClientId](set-client-id.md)(@NonNullclientId: String): [PassportTurboAppAuthProperties.Builder](index.md)<br>clientId oauth приложения |
| [setEnvironment](set-environment.md) | [passport]<br>abstract fun [setEnvironment](set-environment.md)(@NonNullenvironment: [PassportEnvironment](../../-passport-environment/index.md)): [PassportTurboAppAuthProperties.Builder](index.md)<br>Окружение в котором должна происходить авторизация. |
| [setScopes](set-scopes.md) | [passport]<br>abstract fun [setScopes](set-scopes.md)(@NonNullscopes: List&lt;String&gt;): [PassportTurboAppAuthProperties.Builder](index.md)<br>Список скоупов, которые нужно запросить. |
| [setTheme](set-theme.md) | [passport]<br>abstract fun [setTheme](set-theme.md)(@NonNulltheme: [PassportTheme](../../-passport-theme/index.md)): [PassportTurboAppAuthProperties.Builder](index.md) |
| [setTurboAppIdentifier](set-turbo-app-identifier.md) | [passport]<br>abstract fun [setTurboAppIdentifier](set-turbo-app-identifier.md)(@NonNullturboAppIdentifier: String): [PassportTurboAppAuthProperties.Builder](index.md)<br>Идентификатор турбоаппа. |
| [setUid](set-uid.md) | [passport]<br>abstract fun [setUid](set-uid.md)(@Nullableuid: [PassportUid](../../-passport-uid/index.md)): [PassportTurboAppAuthProperties.Builder](index.md)<br>Uid аккаунта который нужно использовать для авторизации в турбоаппе. |
