//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactSyncFacade](index.md)

# ContactSyncFacade

[passport-contacts-core]\
class [ContactSyncFacade](index.md)

Facade class for actually synchronizing contacts.

## Functions

| Name | Summary |
|---|---|
| [sync](sync.md) | [passport-contacts-core]<br>@RequiresPermission(value = &quot;android.permission.READ_CONTACTS&quot;)<br>suspend fun [sync](sync.md)(): Result&lt;[ContactsSyncResult](../-contacts-sync-result/index.md)&gt;<br>Synchronize contacts in suspendable coroutine. |
| [syncBlocking](sync-blocking.md) | [passport-contacts-core]<br>@WorkerThread<br>@RequiresPermission(value = &quot;android.permission.READ_CONTACTS&quot;)<br>fun [syncBlocking](sync-blocking.md)(): [ContactsSyncResult](../-contacts-sync-result/index.md)<br>Synchronize contacts, blocking the thread it was invoked. NB! Never call in main thread!!! |
