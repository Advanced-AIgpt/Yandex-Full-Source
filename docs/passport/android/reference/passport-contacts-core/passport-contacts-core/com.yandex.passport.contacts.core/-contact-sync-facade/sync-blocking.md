//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactSyncFacade](index.md)/[syncBlocking](sync-blocking.md)

# syncBlocking

[passport-contacts-core]\

@WorkerThread

@RequiresPermission(value = &quot;android.permission.READ_CONTACTS&quot;)

fun [syncBlocking](sync-blocking.md)(): [ContactsSyncResult](../-contacts-sync-result/index.md)

Synchronize contacts, blocking the thread it was invoked. NB! Never call in main thread!!!

#### Return

[ContactsSyncResult](../-contacts-sync-result/index.md).
