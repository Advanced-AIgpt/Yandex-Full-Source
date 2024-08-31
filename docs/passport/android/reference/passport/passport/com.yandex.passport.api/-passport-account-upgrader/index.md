//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccountUpgrader](index.md)

# PassportAccountUpgrader

[passport]\
interface [PassportAccountUpgrader](index.md)

A facade interface for mechanism of account upgrade.

- 
   host is responsible for getting [PassportAccountUpgradeStatus](../-passport-account-upgrade-status/index.md) for the account it is using.
- 
   When returned status has [PassportAccountUpgradeStatus.shouldAccountBeUpgraded](../should-account-be-upgraded.md) as true, host must call [createUpgradeIntent](create-upgrade-intent.md) and start it as Activity.

Not designed to use from java: use [PassportAccountUpgraderJavaCompat](../../com.yandex.passport.api.javacompat/-passport-account-upgrader-java-compat/index.md) for oldtimer java clients.

## Types

| Name | Summary |
|---|---|
| [UpdateInterval](-update-interval/index.md) | [passport]<br>enum [UpdateInterval](-update-interval/index.md) : Enum&lt;[PassportAccountUpgrader.UpdateInterval](-update-interval/index.md)&gt; <br>Allows to specify time interval for required status update. |

## Functions

| Name | Summary |
|---|---|
| [createUpgradeIntent](create-upgrade-intent.md) | [passport]<br>abstract suspend fun [createUpgradeIntent](create-upgrade-intent.md)(context: Context, properties: [UpgradeProperties](../-upgrade-properties/index.md)): Intent?<br>Start the data replenishment process by providing an Intent to be used by the host by calling startActivity. |
| [getUpgradeStatus](get-upgrade-status.md) | [passport]<br>abstract fun [getUpgradeStatus](get-upgrade-status.md)(uid: [PassportUid](../-passport-uid/index.md), updateInterval: [PassportAccountUpgrader.UpdateInterval](-update-interval/index.md) = UpdateInterval.ONE_DAY): Flow&lt;[PassportAccountUpgradeStatus](../-passport-account-upgrade-status/index.md)&gt;<br>Gets the Flow with upgrade status fow given [uid](get-upgrade-status.md). First emitted value is always the cached one. |
| [userDeclinedUpgrade](user-declined-upgrade.md) | [passport]<br>abstract suspend fun [userDeclinedUpgrade](user-declined-upgrade.md)(uid: [PassportUid](../-passport-uid/index.md))<br>Methods for notifying Passport that the user refused to fill in the data. It must be called if the host shows its additional UI with the status before filling in the data and a dialog with a suggestion to fill in the data with the option to refuse. |
