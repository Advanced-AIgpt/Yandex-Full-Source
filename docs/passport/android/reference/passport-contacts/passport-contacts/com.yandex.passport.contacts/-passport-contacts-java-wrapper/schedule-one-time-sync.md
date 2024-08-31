//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsJavaWrapper](index.md)/[scheduleOneTimeSync](schedule-one-time-sync.md)

# scheduleOneTimeSync

[passport-contacts]\

@JvmOverloads

fun [scheduleOneTimeSync](schedule-one-time-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP, callback: [PassportContactsJavaWrapper.ScheduleCallback](-schedule-callback/index.md)): [Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md)

Try to schedule an one time request.

#### Return

[Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md) to close subscription.

## See also

passport-contacts

| | |
|---|---|
| [com.yandex.passport.contacts.PassportContactsApi](../-passport-contacts-api/schedule-one-time-sync.md) |  |

## Parameters

passport-contacts

| | |
|---|---|
| callback | to get result as [PassportContactsScheduleStatus](../-passport-contacts-schedule-status/index.md). |
