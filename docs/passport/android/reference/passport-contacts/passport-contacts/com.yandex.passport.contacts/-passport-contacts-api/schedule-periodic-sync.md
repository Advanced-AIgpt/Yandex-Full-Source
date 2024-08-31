//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsApi](index.md)/[schedulePeriodicSync](schedule-periodic-sync.md)

# schedulePeriodicSync

[passport-contacts]\
abstract suspend fun [schedulePeriodicSync](schedule-periodic-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, period: [PassportContactsPeriod](../-passport-contacts-period/index.md) = PassportContactsPeriod.DAILY, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP): [PassportContactsScheduleStatus](../-passport-contacts-schedule-status/index.md)

Try to schedule an periodic sync request. If contact permission is not yet granted by user a [permissionRationale](schedule-periodic-sync.md) will be shown and permission will be requested as well. After this an androidx.work.OneTimeWorkRequest will be scheduled using androidx.work.WorkManager.

## Parameters

passport-contacts

| | |
|---|---|
| activity | a ComponentActivity descendant instance to use for show rationale and request permissions. |
| permissionRationale | to show when asking user for contacts permission |
| period | to repeat sync requests every time |
| existingWorkPolicy | to use when sync job was already scheduled before |
