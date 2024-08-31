//[passport](../../../index.md)/[com.yandex.passport.api.javacompat](../index.md)/[PassportAccountUpgraderJavaCompat](index.md)/[createUpgradeIntent](create-upgrade-intent.md)

# createUpgradeIntent

[passport]\

@WorkerThread

fun [createUpgradeIntent](create-upgrade-intent.md)(context: Context, properties: [UpgradeProperties](../../com.yandex.passport.api/-upgrade-properties/index.md)): Intent?

Start the data replenishment process by providing an Intent to be used by the host by calling startActivity.

Important! Must be called from worker thread only as it will be waiting for status checking. NB! Do not forget to switch back to Ui thread before calling startActivity with result returned.

#### Return

an Intent for startActivity calling or null when data replenishment process is not required.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportAccountUpgrader](../../com.yandex.passport.api/-passport-account-upgrader/create-upgrade-intent.md) |  |
