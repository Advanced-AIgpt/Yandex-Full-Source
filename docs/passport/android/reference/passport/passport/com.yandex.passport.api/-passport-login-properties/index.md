//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportLoginProperties](index.md)

# PassportLoginProperties

[passport]\
interface [PassportLoginProperties](index.md)

Contains a filter for showing the authorization form to the user.

## Types

| Name | Summary |
|---|---|
| [Builder](-builder/index.md) | [passport]<br>interface [Builder](-builder/index.md) : [PassportLoginProperties](index.md) |

## Properties

| Name | Summary |
|---|---|
| [analyticsParams](analytics-params.md) | [passport]<br>abstract val [analyticsParams](analytics-params.md): Map&lt;String, String&gt;<br>Parameters that will be sent to yandex.metrica along with the authorization screen events. |
| [animationTheme](animation-theme.md) | [passport]<br>abstract val [animationTheme](animation-theme.md): [PassportAnimationTheme](../-passport-animation-theme/index.md)?<br>Use to override passport animations. |
| [bindPhoneProperties](bind-phone-properties.md) | [passport]<br>abstract val [bindPhoneProperties](bind-phone-properties.md): [PassportBindPhoneProperties](../-passport-bind-phone-properties/index.md)? |
| [filter](filter.md) | [passport]<br>abstract val [filter](filter.md): [PassportFilter](../-passport-filter/index.md)<br>Contains a filter of accounts by their type. Required. |
| [isAdditionOnlyRequired](is-addition-only-required.md) | [passport]<br>abstract val [isAdditionOnlyRequired](is-addition-only-required.md): Boolean<br>Do not show existing accounts, only adding a new account. |
| [isRegistrationOnlyRequired](is-registration-only-required.md) | [passport]<br>abstract val [isRegistrationOnlyRequired](is-registration-only-required.md): Boolean<br>Do not show existing accounts or adding a new account, only registering a new user. |
| [loginHint](login-hint.md) | [passport]<br>abstract val [loginHint](login-hint.md): String?<br>It will be used only if the [selectedUid](selected-uid.md) parameter is passed. |
| [selectedUid](selected-uid.md) | [passport]<br>abstract val [selectedUid](selected-uid.md): [PassportUid](../-passport-uid/index.md)?<br>Use to select an account. |
| [socialConfiguration](social-configuration.md) | [passport]<br>abstract val [socialConfiguration](social-configuration.md): [PassportSocialConfiguration](../-passport-social-configuration/index.md)?<br>Show authorization only for the selected social network or mail provider. |
| [socialRegistrationProperties](social-registration-properties.md) | [passport]<br>abstract val [socialRegistrationProperties](social-registration-properties.md): [PassportSocialRegistrationProperties](../-passport-social-registration-properties/index.md)<br>Sets parameters for finishing a social account registration. |
| [source](source.md) | [passport]<br>abstract val [source](source.md): String?<br>A certain string by which the source of the authorization can be identified. |
| [theme](theme.md) | [passport]<br>abstract val [theme](theme.md): [PassportTheme](../-passport-theme/index.md)<br>Define theme as [PassportTheme](../-passport-theme/index.md). |
| [turboAuthParams](turbo-auth-params.md) | [passport]<br>abstract val [turboAuthParams](turbo-auth-params.md): [PassportTurboAuthParams](../-passport-turbo-auth-params/index.md)? |
| [visualProperties](visual-properties.md) | [passport]<br>abstract val [visualProperties](visual-properties.md): [PassportVisualProperties](../-passport-visual-properties/index.md)<br>Define the customization of the authorization screen. |
| [webAmProperties](web-am-properties.md) | [passport]<br>abstract val [webAmProperties](web-am-properties.md): [PassportWebAmProperties](../-passport-web-am-properties/index.md)? |

## Inheritors

| Name |
|---|
| [Builder](-builder/index.md) |
