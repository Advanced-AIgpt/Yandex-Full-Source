//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [addCredentials](add-credentials.md) | [passport]<br>abstract fun [addCredentials](add-credentials.md)(environment: [PassportEnvironment](../../-passport-environment/index.md), credentials: [PassportCredentials](../../-passport-credentials/index.md)): [PassportProperties.Builder](index.md)<br>Добавить зашифрованную пару client id/secret [credentials](../../-passport-credentials/index.md) для окружения [environment](../../-passport-environment/index.md).<br></br><br>[passport]<br>abstract fun [addCredentials](add-credentials.md)(environment: [KPassportEnvironment](../../-k-passport-environment/index.md), encryptedId: String, encryptedSecret: String) |
| [addMasterCredentials](add-master-credentials.md) | [passport]<br>abstract fun [addMasterCredentials](add-master-credentials.md)(environment: [PassportEnvironment](../../-passport-environment/index.md), credentials: [PassportCredentials](../../-passport-credentials/index.md)): [PassportProperties.Builder](index.md)<br>Метод не должен использоваться приложениями без согласования с командой паспорта.<br>[passport]<br>abstract fun [addMasterCredentials](add-master-credentials.md)(environment: [KPassportEnvironment](../../-k-passport-environment/index.md), encryptedId: String, encryptedSecret: String) |
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportProperties](../index.md) |
| [defaultLoginProperties](default-login-properties.md) | [passport]<br>abstract fun [defaultLoginProperties](default-login-properties.md)(init: [PassportLoginProperties.Builder](../../-passport-login-properties/-builder/index.md).() -&gt; Unit) |
| [enablePushNotifications](enable-push-notifications.md) | [passport]<br>abstract fun [enablePushNotifications](enable-push-notifications.md)(pushTokenProvider: [PassportPushTokenProvider](../../-passport-push-token-provider/index.md)): [PassportProperties.Builder](index.md)<br>Включает уведомления о подозрительном входе. Необходимо только для почты. <br></br> |
| [setAccountSharingEnabled](set-account-sharing-enabled.md) | [passport]<br>abstract fun [setAccountSharingEnabled](set-account-sharing-enabled.md)(enabled: Boolean?): [PassportProperties.Builder](index.md)<br>Включает или отключает обмен аккаунтами с другими accountType. Нельзя использовавать без согласования с командой паспорта. |
| [setApplicationClid](set-application-clid.md) | [passport]<br>abstract fun [setApplicationClid](set-application-clid.md)(applicationClid: String): [PassportProperties.Builder](index.md)<br>Определить clid.<br></br> |
| [setAssertionDelegate](set-assertion-delegate.md) | [passport]<br>abstract fun [setAssertionDelegate](set-assertion-delegate.md)(delegate: [PassportAssertionDelegate](../../-passport-assertion-delegate/index.md)?): [PassportProperties.Builder](index.md) |
| [setBackendHost](set-backend-host.md) | [passport]<br>abstract fun [setBackendHost](set-backend-host.md)(host: String): [PassportProperties.Builder](index.md)<br>Определить хост API. По-умолчанию сейчас это <tt>mobileproxy.passport.yandex.net</tt>.<br></br> |
| [setDefaultLoginProperties](set-default-login-properties.md) | [passport]<br>abstract fun [setDefaultLoginProperties](set-default-login-properties.md)(loginProperties: [PassportLoginProperties](../../-passport-login-properties/index.md)?): [PassportProperties.Builder](index.md)<br>Задает LoginProperties, которые будут использованы при открытии экрана добавления аккаунта из настроек. Нельзя использовать без согласования с командой паспорта. |
| [setDeviceGeoLocation](set-device-geo-location.md) | [passport]<br>abstract fun [setDeviceGeoLocation](set-device-geo-location.md)(deviceGeoLocation: String): [PassportProperties.Builder](index.md)<br>Определить geo location.<br></br> |
| [setFrontendUrlOverride](set-frontend-url-override.md) | [passport]<br>abstract fun [setFrontendUrlOverride](set-frontend-url-override.md)(override: String?): [PassportProperties.Builder](index.md) |
| [setLegalConfidentialUrl](set-legal-confidential-url.md) | [passport]<br>abstract fun [setLegalConfidentialUrl](set-legal-confidential-url.md)(legalConfidentialUrl: String): [PassportProperties.Builder](index.md)<br>Определить URL политики конфиденциальности.<br></br><br></br> По-умолчанию ключ <tt>reg_account_eula_confidential_url</tt> в Танкере, проект <tt>https://tanker.yandex-team.ru/?project=mobile-common-android-auth&branch=master&keyset=strings Для русского языка сейчас это <tt>https://yandex.ru/legal/confidential/?mode=html, для английского <tt>https://yandex.com/legal/confidential/?mode=html |
| [setLegalRulesUrl](set-legal-rules-url.md) | [passport]<br>abstract fun [setLegalRulesUrl](set-legal-rules-url.md)(legalRulesUrl: String): [PassportProperties.Builder](index.md)<br>Определить URL пользовательского соглашения при регистрации пользователя.<br></br><br></br> По-умолчанию ключ <tt>reg_account_eula_user_agreement_url</tt> в Танкере, проект <tt>https://tanker.yandex-team.ru/?project=mobile-common-android-auth&branch=master&keyset=strings Для русского языка сейчас это <tt>https://yandex.ru/legal/rules/?lang=ru&mode=html, для английского <tt>https://yandex.com/legal/rules/?lang=en&mode=html |
| [setLogger](set-logger.md) | [passport]<br>abstract fun [setLogger](set-logger.md)(logger: [PassportLogger](../../-passport-logger/index.md)?): [PassportProperties.Builder](index.md) |
| [setOkHttpClientBuilder](set-ok-http-client-builder.md) | [passport]<br>abstract fun [setOkHttpClientBuilder](set-ok-http-client-builder.md)(okHttpClientBuilder: OkHttpClient.Builder): [PassportProperties.Builder](index.md)<br>Определить OkHttpClient.Builder. Удобно использовать для отладки и при необходимости переопределить компоненты сетевого уровня.<br></br><br></br> `<pre> OkHttpClient.Builder okHttpClientBuilder = new OkHttpClient.Builder() .dns(hostname -> { return Dns.SYSTEM.lookup(hostname); }) .addNetworkInterceptor(new HttpLoggingInterceptor(message -> Log.d(&quot;Network&quot;, message)) .setLevel(HttpLoggingInterceptor.Level.BODY)) .addNetworkInterceptor(new StethoInterceptor()); |
| [setPreferredLocale](set-preferred-locale.md) | [passport]<br>abstract fun [setPreferredLocale](set-preferred-locale.md)(locale: Locale?): [PassportProperties.Builder](index.md) |
| [setWebLoginUrlOverride](set-web-login-url-override.md) | [passport]<br>abstract fun [setWebLoginUrlOverride](set-web-login-url-override.md)(override: String?): [PassportProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [applicationClid](application-clid.md) | [passport]<br>abstract override var [applicationClid](application-clid.md): String?<br>Определяет clid. |
| [assertionDelegate](assertion-delegate.md) | [passport]<br>abstract override var [assertionDelegate](assertion-delegate.md): [PassportAssertionDelegate](../../-passport-assertion-delegate/index.md)?<br>Delegate, allowing to override default assertion failure behaviour. |
| [backendHost](backend-host.md) | [passport]<br>abstract override var [backendHost](backend-host.md): String?<br>Определяет хост API. По-умолчанию сейчас это <tt>mobileproxy.passport.yandex.net</tt>. |
| [credentialsMap](credentials-map.md) | [passport]<br>abstract override var [credentialsMap](credentials-map.md): MutableMap&lt;[PassportEnvironment](../../-passport-environment/index.md), [PassportCredentials](../../-passport-credentials/index.md)&gt;<br>Отображение зашифрованной пары client id/secret [credentials](../../-passport-credentials/index.md) к окружению [environment](../../-passport-environment/index.md). |
| [defaultLoginProperties](default-login-properties.md) | [passport]<br>abstract override var [defaultLoginProperties](default-login-properties.md): [PassportLoginProperties](../../-passport-login-properties/index.md)?<br>Задает LoginProperties, которые будут использованы при открытии экрана добавления аккаунта из настроек. Нельзя использовать без согласования с командой паспорта. |
| [deviceGeoLocation](device-geo-location.md) | [passport]<br>abstract override var [deviceGeoLocation](device-geo-location.md): String?<br>Определяет геолокацию устройства. |
| [frontendUrlOverride](frontend-url-override.md) | [passport]<br>abstract override var [frontendUrlOverride](frontend-url-override.md): String?<br>An override url for passport frontend. |
| [isAccountSharingEnabled](is-account-sharing-enabled.md) | [passport]<br>abstract override var [isAccountSharingEnabled](is-account-sharing-enabled.md): Boolean?<br>Включает или отключает обмен аккаунтами с другими accountType. Нельзя использовавать без согласования с командой паспорта. |
| [isPushNotificationsEnabled](is-push-notifications-enabled.md) | [passport]<br>open override val [isPushNotificationsEnabled](is-push-notifications-enabled.md): Boolean<br>Включает уведомления о подозрительном входе. Необходимо только для почты. |
| [legalConfidentialUrl](legal-confidential-url.md) | [passport]<br>abstract override var [legalConfidentialUrl](legal-confidential-url.md): String?<br>Определить URL политики конфиденциальности. |
| [legalRulesUrl](legal-rules-url.md) | [passport]<br>abstract override var [legalRulesUrl](legal-rules-url.md): String?<br>Определить URL пользовательского соглашения при регистрации пользователя. |
| [logger](logger.md) | [passport]<br>abstract override var [logger](logger.md): [PassportLogger](../../-passport-logger/index.md)?<br>Allows to override default system logging. |
| [masterCredentialsMap](master-credentials-map.md) | [passport]<br>abstract override var [masterCredentialsMap](master-credentials-map.md): MutableMap&lt;[PassportEnvironment](../../-passport-environment/index.md), [PassportCredentials](../../-passport-credentials/index.md)&gt;<br>Не должено использоваться приложениями без согласования с командой паспорта. |
| [okHttpClientBuilder](ok-http-client-builder.md) | [passport]<br>abstract override var [okHttpClientBuilder](ok-http-client-builder.md): OkHttpClient.Builder<br>Определяет OkHttpClient.Builder. Удобно использовать для отладки и при необходимости переопределить компоненты сетевого уровня. |
| [preferredLocale](preferred-locale.md) | [passport]<br>abstract override var [preferredLocale](preferred-locale.md): Locale?<br>Preferred locale to use. |
| [pushTokenProvider](push-token-provider.md) | [passport]<br>abstract override var [pushTokenProvider](push-token-provider.md): [PassportPushTokenProvider](../../-passport-push-token-provider/index.md)?<br>Объект, возвращающий gcm (fcm) токен. |
| [webLoginUrlOverride](web-login-url-override.md) | [passport]<br>abstract override var [webLoginUrlOverride](web-login-url-override.md): String?<br>An override url for passport web login. |
