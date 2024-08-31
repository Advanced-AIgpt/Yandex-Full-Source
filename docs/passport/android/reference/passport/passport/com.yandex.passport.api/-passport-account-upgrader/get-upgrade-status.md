//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccountUpgrader](index.md)/[getUpgradeStatus](get-upgrade-status.md)

# getUpgradeStatus

[passport]\
abstract fun [getUpgradeStatus](get-upgrade-status.md)(uid: [PassportUid](../-passport-uid/index.md), updateInterval: [PassportAccountUpgrader.UpdateInterval](-update-interval/index.md) = UpdateInterval.ONE_DAY): Flow&lt;[PassportAccountUpgradeStatus](../-passport-account-upgrade-status/index.md)&gt;

Gets the Flow with upgrade status fow given [uid](get-upgrade-status.md). First emitted value is always the cached one.

## Parameters

passport

| | |
|---|---|
| updateInterval | allows to override an interval to update status values. Interval is approximate and not guaranteed by passport to be exactly matched. |
