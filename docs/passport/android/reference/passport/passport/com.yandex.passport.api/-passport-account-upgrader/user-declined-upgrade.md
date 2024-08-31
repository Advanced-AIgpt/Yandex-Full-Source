//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccountUpgrader](index.md)/[userDeclinedUpgrade](user-declined-upgrade.md)

# userDeclinedUpgrade

[passport]\
abstract suspend fun [userDeclinedUpgrade](user-declined-upgrade.md)(uid: [PassportUid](../-passport-uid/index.md))

Methods for notifying Passport that the user refused to fill in the data. It must be called if the host shows its additional UI with the status before filling in the data and a dialog with a suggestion to fill in the data with the option to refuse.
