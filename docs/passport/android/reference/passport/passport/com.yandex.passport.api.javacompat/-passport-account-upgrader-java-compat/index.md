//[passport](../../../index.md)/[com.yandex.passport.api.javacompat](../index.md)/[PassportAccountUpgraderJavaCompat](index.md)

# PassportAccountUpgraderJavaCompat

[passport]\
class [PassportAccountUpgraderJavaCompat](index.md)(passportAccountUpgrader: [PassportAccountUpgrader](../../com.yandex.passport.api/-passport-account-upgrader/index.md))

Java-compatible wrapper for [PassportAccountUpgrader](../../com.yandex.passport.api/-passport-account-upgrader/index.md). For still-Java retro client hosts.

## Constructors

| | |
|---|---|
| [PassportAccountUpgraderJavaCompat](-passport-account-upgrader-java-compat.md) | [passport]<br>fun [PassportAccountUpgraderJavaCompat](-passport-account-upgrader-java-compat.md)(passportAccountUpgrader: [PassportAccountUpgrader](../../com.yandex.passport.api/-passport-account-upgrader/index.md)) |

## Types

| Name | Summary |
|---|---|
| [Listener](-listener/index.md) | [passport]<br>fun interface [Listener](-listener/index.md)<br>Listener to get status changes. |

## Functions

| Name | Summary |
|---|---|
| [createUpgradeIntent](create-upgrade-intent.md) | [passport]<br>@WorkerThread<br>fun [createUpgradeIntent](create-upgrade-intent.md)(context: Context, properties: [UpgradeProperties](../../com.yandex.passport.api/-upgrade-properties/index.md)): Intent?<br>Start the data replenishment process by providing an Intent to be used by the host by calling startActivity. |
| [getCurrentUpgradeStatus](get-current-upgrade-status.md) | [passport]<br>@WorkerThread<br>fun [getCurrentUpgradeStatus](get-current-upgrade-status.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md)): [PassportAccountUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrade-status/index.md)<br>Gets the first status from [PassportAccountUpgrader.getUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrader/get-upgrade-status.md). |
| [subscribeForStatus](subscribe-for-status.md) | [passport]<br>fun [subscribeForStatus](subscribe-for-status.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md), listener: [PassportAccountUpgraderJavaCompat.Listener](-listener/index.md)): [Disposable](../../com.yandex.passport.common/-disposable/index.md)<br>Listener-based wrapper for [PassportAccountUpgrader.getUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrader/get-upgrade-status.md). All status updates are represented by events fired to listener.<br>[passport]<br>fun [subscribeForStatus](subscribe-for-status.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md), updateInterval: [PassportAccountUpgrader.UpdateInterval](../../com.yandex.passport.api/-passport-account-upgrader/-update-interval/index.md), listener: [PassportAccountUpgraderJavaCompat.Listener](-listener/index.md)): [Disposable](../../com.yandex.passport.common/-disposable/index.md)<br>Listener-based wrapper for [PassportAccountUpgrader.getUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrader/get-upgrade-status.md). All status updates are represented by events fired to listener. Allows to specify updateInterval. |
| [userDeclinedUpgrade](user-declined-upgrade.md) | [passport]<br>@WorkerThread<br>fun [userDeclinedUpgrade](user-declined-upgrade.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md)) |
