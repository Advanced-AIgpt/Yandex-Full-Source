//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportVisualProperties](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportVisualProperties](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportVisualProperties](../index.md) |
| [disableSocialAuthorization](disable-social-authorization.md) | [passport]<br>abstract fun [disableSocialAuthorization](disable-social-authorization.md)(): [PassportVisualProperties.Builder](index.md) |
| [hideBackButton](hide-back-button.md) | [passport]<br>~~abstract~~ ~~fun~~ [~~hideBackButton~~](hide-back-button.md)~~(~~~~)~~~~:~~ [PassportVisualProperties.Builder](index.md) |
| [hideChoosingAnotherAccountOnReloginButton](hide-choosing-another-account-on-relogin-button.md) | [passport]<br>abstract fun [hideChoosingAnotherAccountOnReloginButton](hide-choosing-another-account-on-relogin-button.md)(): [PassportVisualProperties.Builder](index.md) |
| [noReturnToHost](no-return-to-host.md) | [passport]<br>abstract fun [noReturnToHost](no-return-to-host.md)(): [PassportVisualProperties.Builder](index.md) |
| [setAuthMessage](set-auth-message.md) | [passport]<br>abstract fun [setAuthMessage](set-auth-message.md)(authMessage: String): [PassportVisualProperties.Builder](index.md) |
| [setCustomLogoText](set-custom-logo-text.md) | [passport]<br>abstract fun [setCustomLogoText](set-custom-logo-text.md)(text: String?): [PassportVisualProperties.Builder](index.md) |
| [setDeleteAccountMessage](set-delete-account-message.md) | [passport]<br>abstract fun [setDeleteAccountMessage](set-delete-account-message.md)(deleteAccountMessage: String): [PassportVisualProperties.Builder](index.md) |
| [setIdentifierHintVariant](set-identifier-hint-variant.md) | [passport]<br>abstract fun [setIdentifierHintVariant](set-identifier-hint-variant.md)(variant: [PassportIdentifierHintVariant](../../-passport-identifier-hint-variant/index.md)): [PassportVisualProperties.Builder](index.md) |
| [setPreferPhonishAuth](set-prefer-phonish-auth.md) | [passport]<br>abstract fun [setPreferPhonishAuth](set-prefer-phonish-auth.md)(prefer: Boolean): [PassportVisualProperties.Builder](index.md) |
| [setRegistrationMessage](set-registration-message.md) | [passport]<br>abstract fun [setRegistrationMessage](set-registration-message.md)(registrationMessage: String): [PassportVisualProperties.Builder](index.md) |
| [setUsernameMessage](set-username-message.md) | [passport]<br>abstract fun [setUsernameMessage](set-username-message.md)(usernameMessage: String): [PassportVisualProperties.Builder](index.md) |
| [showSkipButton](show-skip-button.md) | [passport]<br>abstract fun [showSkipButton](show-skip-button.md)(): [PassportVisualProperties.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [authMessage](auth-message.md) | [passport]<br>abstract override var [authMessage](auth-message.md): String?<br>Sets the message displayed to the user in the authorization screen. |
| [customLogoText](custom-logo-text.md) | [passport]<br>abstract override var [customLogoText](custom-logo-text.md): String?<br>Used to set custom logo text. |
| [deleteAccountMessage](delete-account-message.md) | [passport]<br>abstract override var [deleteAccountMessage](delete-account-message.md): String?<br>Sets the message shown to the user when trying to delete an account. The text must contain the string <tt>%1$s</tt>, which will be replaced with the username. |
| [identifierHintVariant](identifier-hint-variant.md) | [passport]<br>abstract override var [identifierHintVariant](identifier-hint-variant.md): [PassportIdentifierHintVariant](../../-passport-identifier-hint-variant/index.md)<br>Show the corresponding hint in the input field of the authorization form. |
| [isBackButtonHidden](is-back-button-hidden.md) | [passport]<br>~~open~~ ~~override~~ ~~var~~ [~~isBackButtonHidden~~](is-back-button-hidden.md)~~:~~ Boolean<br>Hide the &quot;back arrow&quot; in the header of the authorization window. |
| [isChoosingAnotherAccountOnReloginButtonHidden](is-choosing-another-account-on-relogin-button-hidden.md) | [passport]<br>abstract override var [isChoosingAnotherAccountOnReloginButtonHidden](is-choosing-another-account-on-relogin-button-hidden.md): Boolean<br>Hide the &quot;Choose another account&quot; button on the re-authorization screen. |
| [isNoReturnToHost](is-no-return-to-host.md) | [passport]<br>abstract override var [isNoReturnToHost](is-no-return-to-host.md): Boolean<br>Disable possibilities to return back to host when auth is shown. |
| [isPreferPhonishAuth](is-prefer-phonish-auth.md) | [passport]<br>abstract override var [isPreferPhonishAuth](is-prefer-phonish-auth.md): Boolean<br>When you start authorization, the finish authorization screen with the login button via yandex will be shown first of all. |
| [isSkipButtonShown](is-skip-button-shown.md) | [passport]<br>abstract override var [isSkipButtonShown](is-skip-button-shown.md): Boolean<br>Show the &quot;Skip&quot; button in the authorization window header to scroll through the login screen without authorization. |
| [isSocialAuthorizationEnabled](is-social-authorization-enabled.md) | [passport]<br>abstract override var [isSocialAuthorizationEnabled](is-social-authorization-enabled.md): Boolean<br>Used to disable the display of social buttons in authorization form. When disabled, filters out social accounts in the account list. |
| [registrationMessage](registration-message.md) | [passport]<br>abstract override var [registrationMessage](registration-message.md): String?<br>Sets the message displayed to the user in the phone input screen at the start of registration. |
| [usernameMessage](username-message.md) | [passport]<br>abstract override var [usernameMessage](username-message.md): String?<br>Sets the message displayed to the user in the first and last name input screen. |
