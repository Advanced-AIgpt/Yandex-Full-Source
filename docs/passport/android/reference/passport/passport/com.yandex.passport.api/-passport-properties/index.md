//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportProperties](index.md)

# PassportProperties

[passport]\
interface [PassportProperties](index.md)

Содержит параметры начальной инициализации библиотеки.

## Types

| Name | Summary |
|---|---|
| [Builder](-builder/index.md) | [passport]<br>interface [Builder](-builder/index.md) : [PassportProperties](index.md) |

## Properties

| Name | Summary |
|---|---|
| [applicationClid](application-clid.md) | [passport]<br>abstract val [applicationClid](application-clid.md): String?<br>Определяет clid. |
| [assertionDelegate](assertion-delegate.md) | [passport]<br>abstract val [assertionDelegate](assertion-delegate.md): [PassportAssertionDelegate](../-passport-assertion-delegate/index.md)?<br>Delegate, allowing to override default assertion failure behaviour. |
| [backendHost](backend-host.md) | [passport]<br>abstract val [backendHost](backend-host.md): String?<br>Определяет хост API. По-умолчанию сейчас это <tt>mobileproxy.passport.yandex.net</tt>. |
| [credentialsMap](credentials-map.md) | [passport]<br>abstract val [credentialsMap](credentials-map.md): Map&lt;[PassportEnvironment](../-passport-environment/index.md), [PassportCredentials](../-passport-credentials/index.md)&gt;<br>Отображение зашифрованной пары client id/secret [credentials](../-passport-credentials/index.md) к окружению [environment](../-passport-environment/index.md). |
| [defaultLoginProperties](default-login-properties.md) | [passport]<br>abstract val [defaultLoginProperties](default-login-properties.md): [PassportLoginProperties](../-passport-login-properties/index.md)?<br>Задает LoginProperties, которые будут использованы при открытии экрана добавления аккаунта из настроек. Нельзя использовать без согласования с командой паспорта. |
| [deviceGeoLocation](device-geo-location.md) | [passport]<br>abstract val [deviceGeoLocation](device-geo-location.md): String?<br>Определяет геолокацию устройства. |
| [frontendUrlOverride](frontend-url-override.md) | [passport]<br>abstract val [frontendUrlOverride](frontend-url-override.md): String?<br>An override url for passport frontend. |
| [isAccountSharingEnabled](is-account-sharing-enabled.md) | [passport]<br>abstract val [isAccountSharingEnabled](is-account-sharing-enabled.md): Boolean?<br>Включает или отключает обмен аккаунтами с другими accountType. Нельзя использовавать без согласования с командой паспорта. |
| [isPushNotificationsEnabled](is-push-notifications-enabled.md) | [passport]<br>abstract val [isPushNotificationsEnabled](is-push-notifications-enabled.md): Boolean<br>Включает уведомления о подозрительном входе. Необходимо только для почты. |
| [legalConfidentialUrl](legal-confidential-url.md) | [passport]<br>abstract val [legalConfidentialUrl](legal-confidential-url.md): String?<br>Определить URL политики конфиденциальности. |
| [legalRulesUrl](legal-rules-url.md) | [passport]<br>abstract val [legalRulesUrl](legal-rules-url.md): String?<br>Определить URL пользовательского соглашения при регистрации пользователя. |
| [logger](logger.md) | [passport]<br>abstract val [logger](logger.md): [PassportLogger](../-passport-logger/index.md)?<br>Allows to override default system logging. |
| [masterCredentialsMap](master-credentials-map.md) | [passport]<br>abstract val [masterCredentialsMap](master-credentials-map.md): Map&lt;[PassportEnvironment](../-passport-environment/index.md), [PassportCredentials](../-passport-credentials/index.md)&gt;<br>Не должено использоваться приложениями без согласования с командой паспорта. |
| [okHttpClientBuilder](ok-http-client-builder.md) | [passport]<br>abstract val [okHttpClientBuilder](ok-http-client-builder.md): OkHttpClient.Builder<br>Определяет OkHttpClient.Builder. Удобно использовать для отладки и при необходимости переопределить компоненты сетевого уровня. |
| [preferredLocale](preferred-locale.md) | [passport]<br>abstract val [preferredLocale](preferred-locale.md): Locale?<br>Preferred locale to use. |
| [pushTokenProvider](push-token-provider.md) | [passport]<br>abstract val [pushTokenProvider](push-token-provider.md): [PassportPushTokenProvider](../-passport-push-token-provider/index.md)?<br>Объект, возвращающий gcm (fcm) токен. |
| [webLoginUrlOverride](web-login-url-override.md) | [passport]<br>abstract val [webLoginUrlOverride](web-login-url-override.md): String?<br>An override url for passport web login. |

## Inheritors

| Name |
|---|
| [Builder](-builder/index.md) |
