//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsConfiguration](index.md)

# PassportContactsConfiguration

[passport-contacts]\
data class [PassportContactsConfiguration](index.md)(val context: Context, val useRemoteWorkManager: Boolean = false, val coroutineDispatchers: CoroutineDispatchers = CoroutineDispatchersImpl())

Used to configure an api from host.

## Constructors

| | |
|---|---|
| [PassportContactsConfiguration](-passport-contacts-configuration.md) | [passport-contacts]<br>fun [PassportContactsConfiguration](-passport-contacts-configuration.md)(context: Context, useRemoteWorkManager: Boolean = false, coroutineDispatchers: CoroutineDispatchers = CoroutineDispatchersImpl()) |

## Properties

| Name | Summary |
|---|---|
| [context](context.md) | [passport-contacts]<br>val [context](context.md): Context<br>An application context. |
| [coroutineDispatchers](coroutine-dispatchers.md) | [passport-contacts]<br>val [coroutineDispatchers](coroutine-dispatchers.md): CoroutineDispatchers<br>Optional. Can be used by host to override CoroutineDispatchers, used in suspend coroutine code. |
| [useRemoteWorkManager](use-remote-work-manager.md) | [passport-contacts]<br>val [useRemoteWorkManager](use-remote-work-manager.md): Boolean = false<br>Use true here to use a androidx.work.multiprocess.RemoteWorkManager instead of androidx.work.WorkManager. Setting this flag will allow you to sync contacts in a separate :contactsync process. |
