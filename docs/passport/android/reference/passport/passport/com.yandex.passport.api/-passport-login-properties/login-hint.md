//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportLoginProperties](index.md)/[loginHint](login-hint.md)

# loginHint

[passport]\
abstract val [loginHint](login-hint.md): String?

It will be used only if the [selectedUid](selected-uid.md) parameter is passed.

When relogin, you can pass loginHint, which will be substituted in the login field if the account with the given uid is not found in the local storage.
