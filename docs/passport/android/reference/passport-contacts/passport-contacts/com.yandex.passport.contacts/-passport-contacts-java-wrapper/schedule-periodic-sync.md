//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsJavaWrapper](index.md)/[schedulePeriodicSync](schedule-periodic-sync.md)

# schedulePeriodicSync

[passport-contacts]\

@JvmOverloads

fun [schedulePeriodicSync](schedule-periodic-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, period: [PassportContactsPeriod](../-passport-contacts-period/index.md) = PassportContactsPeriod.DAILY, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP, callback: [PassportContactsJavaWrapper.ScheduleCallback](-schedule-callback/index.md)): [Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md)

Try to schedule an periodic sync request.

#### Return

[Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md) to close subscription.

## See also

passport-contacts

| | |
|---|---|
| [com.yandex.passport.contacts.PassportContactsApi](../-passport-contacts-api/schedule-periodic-sync.md) |  |

## Parameters

passport-contacts

| | |
|---|---|
| callback | to get result as [PassportContactsScheduleStatus](../-passport-contacts-schedule-status/index.md). |
