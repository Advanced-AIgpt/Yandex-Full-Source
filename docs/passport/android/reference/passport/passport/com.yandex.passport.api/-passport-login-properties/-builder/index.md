//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportLoginProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportLoginProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [addAnalyticsParam](add-analytics-param.md) | [passport]<br>abstract fun [addAnalyticsParam](add-analytics-param.md)(key: String, value: String?): [PassportLoginProperties.Builder](index.md) |
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportLoginProperties](../index.md) |
| [requireAdditionOnly](require-addition-only.md) | [passport]<br>abstract fun [requireAdditionOnly](require-addition-only.md)(): [PassportLoginProperties.Builder](index.md) |
| [requireRegistrationOnly](require-registration-only.md) | [passport]<br>abstract fun [requireRegistrationOnly](require-registration-only.md)(): [PassportLoginProperties.Builder](index.md) |
| [selectAccount](select-account.md) | [passport]<br>abstract fun [selectAccount](select-account.md)(uid: [PassportUid](../../-passport-uid/index.md)?): [PassportLoginProperties.Builder](index.md) |
| [setAnimationTheme](set-animation-theme.md) | [passport]<br>abstract fun [setAnimationTheme](set-animation-theme.md)(animationTheme: [PassportAnimationTheme](../../-passport-animation-theme/index.md)): [PassportLoginProperties.Builder](index.md) |
| [setBindPhoneProperties](set-bind-phone-properties.md) | [passport]<br>abstract fun [setBindPhoneProperties](set-bind-phone-properties.md)(passportBindPhoneProperties: [PassportBindPhoneProperties](../../-passport-bind-phone-properties/index.md)): [PassportLoginProperties.Builder](index.md) |
| [setFilter](set-filter.md) | [passport]<br>abstract fun [setFilter](set-filter.md)(filter: [PassportFilter](../../-passport-filter/index.md)): [PassportLoginProperties.Builder](index.md) |
| [setLoginHint](set-login-hint.md) | [passport]<br>abstract fun [setLoginHint](set-login-hint.md)(loginHint: String?): [PassportLoginProperties.Builder](index.md) |
| [setSocialConfiguration](set-social-configuration.md) | [passport]<br>abstract fun [setSocialConfiguration](set-social-configuration.md)(socialConfiguration: [PassportSocialConfiguration](../../-passport-social-configuration/index.md)?): [PassportLoginProperties.Builder](index.md) |
| [setSocialRegistrationProperties](set-social-registration-properties.md) | [passport]<br>abstract fun [setSocialRegistrationProperties](set-social-registration-properties.md)(socialRegistrationProperties: [PassportSocialRegistrationProperties](../../-passport-social-registration-properties/index.md)): [PassportLoginProperties.Builder](index.md) |
| [setSource](set-source.md) | [passport]<br>abstract fun [setSource](set-source.md)(source: String?): [PassportLoginProperties.Builder](index.md) |
| [setTheme](set-theme.md) | [passport]<br>abstract fun [setTheme](set-theme.md)(theme: [PassportTheme](../../-passport-theme/index.md)): [PassportLoginProperties.Builder](index.md) |
| [setTurboAuthParams](set-turbo-auth-params.md) | [passport]<br>abstract fun [setTurboAuthParams](set-turbo-auth-params.md)(turboAuthParams: [PassportTurboAuthParams](../../-passport-turbo-auth-params/index.md)?): [PassportLoginProperties.Builder](index.md) |
| [setVisualProperties](set-visual-properties.md) | [passport]<br>abstract fun [setVisualProperties](set-visual-properties.md)(passportVisualProperties: [PassportVisualProperties](../../-passport-visual-properties/index.md)): [PassportLoginProperties.Builder](index.md) |
| [setWebAmProperties](set-web-am-properties.md) | [passport]<br>abstract fun [setWebAmProperties](set-web-am-properties.md)(value: [PassportWebAmProperties](../../-passport-web-am-properties/index.md)): [PassportLoginProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [analyticsParams](analytics-params.md) | [passport]<br>abstract override var [analyticsParams](analytics-params.md): Map&lt;String, String&gt;<br>Parameters that will be sent to yandex.metrica along with the authorization screen events. |
| [animationTheme](animation-theme.md) | [passport]<br>abstract override var [animationTheme](animation-theme.md): [PassportAnimationTheme](../../-passport-animation-theme/index.md)?<br>Use to override passport animations. |
| [bindPhoneProperties](bind-phone-properties.md) | [passport]<br>abstract override var [bindPhoneProperties](bind-phone-properties.md): [PassportBindPhoneProperties](../../-passport-bind-phone-properties/index.md)? |
| [filter](filter.md) | [passport]<br>abstract override var [filter](filter.md): [PassportFilter](../../-passport-filter/index.md)<br>Contains a filter of accounts by their type. Required. |
| [isAdditionOnlyRequired](is-addition-only-required.md) | [passport]<br>abstract override var [isAdditionOnlyRequired](is-addition-only-required.md): Boolean<br>Do not show existing accounts, only adding a new account. |
| [isRegistrationOnlyRequired](is-registration-only-required.md) | [passport]<br>abstract override var [isRegistrationOnlyRequired](is-registration-only-required.md): Boolean<br>Do not show existing accounts or adding a new account, only registering a new user. |
| [loginHint](login-hint.md) | [passport]<br>abstract override var [loginHint](login-hint.md): String?<br>It will be used only if the [selectedUid](selected-uid.md) parameter is passed. |
| [selectedUid](selected-uid.md) | [passport]<br>abstract override var [selectedUid](selected-uid.md): [PassportUid](../../-passport-uid/index.md)?<br>Use to select an account. |
| [socialConfiguration](social-configuration.md) | [passport]<br>abstract override var [socialConfiguration](social-configuration.md): [PassportSocialConfiguration](../../-passport-social-configuration/index.md)?<br>Show authorization only for the selected social network or mail provider. |
| [socialRegistrationProperties](social-registration-properties.md) | [passport]<br>abstract override var [socialRegistrationProperties](social-registration-properties.md): [PassportSocialRegistrationProperties](../../-passport-social-registration-properties/index.md)<br>Sets parameters for finishing a social account registration. |
| [source](source.md) | [passport]<br>abstract override var [source](source.md): String?<br>A certain string by which the source of the authorization can be identified. |
| [theme](theme.md) | [passport]<br>abstract override var [theme](theme.md): [PassportTheme](../../-passport-theme/index.md)<br>Define theme as [PassportTheme](../../-passport-theme/index.md). |
| [turboAuthParams](turbo-auth-params.md) | [passport]<br>abstract override var [turboAuthParams](turbo-auth-params.md): [PassportTurboAuthParams](../../-passport-turbo-auth-params/index.md)? |
| [visualProperties](visual-properties.md) | [passport]<br>abstract override var [visualProperties](visual-properties.md): [PassportVisualProperties](../../-passport-visual-properties/index.md)<br>Define the customization of the authorization screen. |
| [webAmProperties](web-am-properties.md) | [passport]<br>abstract override var [webAmProperties](web-am-properties.md): [PassportWebAmProperties](../../-passport-web-am-properties/index.md)? |
