//[passport](../../../index.md)/[com.yandex.passport.api.javacompat](../index.md)/[PassportAccountUpgraderJavaCompat](index.md)/[subscribeForStatus](subscribe-for-status.md)

# subscribeForStatus

[passport]\
fun [subscribeForStatus](subscribe-for-status.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md), listener: [PassportAccountUpgraderJavaCompat.Listener](-listener/index.md)): [Disposable](../../com.yandex.passport.common/-disposable/index.md)

Listener-based wrapper for [PassportAccountUpgrader.getUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrader/get-upgrade-status.md). All status updates are represented by events fired to listener.

[passport]\
fun [subscribeForStatus](subscribe-for-status.md)(uid: [PassportUid](../../com.yandex.passport.api/-passport-uid/index.md), updateInterval: [PassportAccountUpgrader.UpdateInterval](../../com.yandex.passport.api/-passport-account-upgrader/-update-interval/index.md), listener: [PassportAccountUpgraderJavaCompat.Listener](-listener/index.md)): [Disposable](../../com.yandex.passport.common/-disposable/index.md)

Listener-based wrapper for [PassportAccountUpgrader.getUpgradeStatus](../../com.yandex.passport.api/-passport-account-upgrader/get-upgrade-status.md). All status updates are represented by events fired to listener. Allows to specify updateInterval.
