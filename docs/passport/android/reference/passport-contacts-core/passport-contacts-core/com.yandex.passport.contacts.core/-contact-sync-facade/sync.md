//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactSyncFacade](index.md)/[sync](sync.md)

# sync

[passport-contacts-core]\

@RequiresPermission(value = &quot;android.permission.READ_CONTACTS&quot;)

suspend fun [sync](sync.md)(): Result&lt;[ContactsSyncResult](../-contacts-sync-result/index.md)&gt;

Synchronize contacts in suspendable coroutine.

#### Return

Result of [ContactsSyncResult](../-contacts-sync-result/index.md).
