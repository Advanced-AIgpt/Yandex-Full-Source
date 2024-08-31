//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccountUpgrader](index.md)/[createUpgradeIntent](create-upgrade-intent.md)

# createUpgradeIntent

[passport]\
abstract suspend fun [createUpgradeIntent](create-upgrade-intent.md)(context: Context, properties: [UpgradeProperties](../-upgrade-properties/index.md)): Intent?

Start the data replenishment process by providing an Intent to be used by the host by calling startActivity.

#### Return

an Intent for startActivity calling or null when data replenishment process is not required.
