//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsApi](index.md)

# PassportContactsApi

[passport-contacts]\
interface [PassportContactsApi](index.md)

Provides api to enqueue synchronization and get status work flow.

## Functions

| Name | Summary |
|---|---|
| [scheduleOneTimeSync](schedule-one-time-sync.md) | [passport-contacts]<br>abstract suspend fun [scheduleOneTimeSync](schedule-one-time-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP): [PassportContactsScheduleStatus](../-passport-contacts-schedule-status/index.md)<br>Try to schedule an one time request. If contact permission is not yet granted by the user a [permissionRationale](schedule-one-time-sync.md) will be shown and permission will be requested as well. After this, an androidx.work.OneTimeWorkRequest will be scheduled using androidx.work.WorkManager. |
| [schedulePeriodicSync](schedule-periodic-sync.md) | [passport-contacts]<br>abstract suspend fun [schedulePeriodicSync](schedule-periodic-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, period: [PassportContactsPeriod](../-passport-contacts-period/index.md) = PassportContactsPeriod.DAILY, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP): [PassportContactsScheduleStatus](../-passport-contacts-schedule-status/index.md)<br>Try to schedule an periodic sync request. If contact permission is not yet granted by user a [permissionRationale](schedule-periodic-sync.md) will be shown and permission will be requested as well. After this an androidx.work.OneTimeWorkRequest will be scheduled using androidx.work.WorkManager. |

## Properties

| Name | Summary |
|---|---|
| [workInfoFlow](work-info-flow.md) | [passport-contacts]<br>abstract val [workInfoFlow](work-info-flow.md): Flow&lt;[PassportContactsWorkInfo](../-passport-contacts-work-info/index.md)&gt;<br>Provides status of work done as flow. |
