//[passport](../../../index.md)/[com.yandex.passport.api.javacompat](../index.md)/[PassportAccountUpgraderJavaCompat](index.md)/[getCurrentUpgradeStatus](get-current-upgrade-status.md)

# getCurrentUpgradeStatus

[passport]\

@WorkerThread

fun [getCurrentUpgradeStatus](get-current-upgrade-status.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md)): [PassportAccountUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrade-status/index.md)

Gets the first status from [PassportAccountUpgrader.getUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrader/get-upgrade-status.md).

Important! Must be called from worker thread only as it will be waiting for needed status.
