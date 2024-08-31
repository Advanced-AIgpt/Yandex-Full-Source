//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsConfiguration](index.md)/[useRemoteWorkManager](use-remote-work-manager.md)

# useRemoteWorkManager

[passport-contacts]\
val [useRemoteWorkManager](use-remote-work-manager.md): Boolean = false

Use true here to use a androidx.work.multiprocess.RemoteWorkManager instead of androidx.work.WorkManager. Setting this flag will allow you to sync contacts in a separate :contactsync process.
