//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsJavaWrapper](index.md)

# PassportContactsJavaWrapper

[passport-contacts]\
class [PassportContactsJavaWrapper](index.md)

Java-wrapper for [PassportContactsApi](../-passport-contacts-api/index.md).

## Types

| Name | Summary |
|---|---|
| [ScheduleCallback](-schedule-callback/index.md) | [passport-contacts]<br>fun interface [ScheduleCallback](-schedule-callback/index.md)<br>Callback used to get result of schedule*-methods. |
| [WorkInfoListener](-work-info-listener/index.md) | [passport-contacts]<br>fun interface [WorkInfoListener](-work-info-listener/index.md)<br>Listener to get updates of [PassportContactsWorkInfo](../-passport-contacts-work-info/index.md). |

## Functions

| Name | Summary |
|---|---|
| [scheduleOneTimeSync](schedule-one-time-sync.md) | [passport-contacts]<br>@JvmOverloads<br>fun [scheduleOneTimeSync](schedule-one-time-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP, callback: [PassportContactsJavaWrapper.ScheduleCallback](-schedule-callback/index.md)): [Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md)<br>Try to schedule an one time request. |
| [schedulePeriodicSync](schedule-periodic-sync.md) | [passport-contacts]<br>@JvmOverloads<br>fun [schedulePeriodicSync](schedule-periodic-sync.md)(activity: ComponentActivity, permissionRationale: [ContactPermissionRationale](../../../../passport-contacts-core/passport-contacts-core/com.yandex.passport.contacts.core/-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, period: [PassportContactsPeriod](../-passport-contacts-period/index.md) = PassportContactsPeriod.DAILY, existingWorkPolicy: [PassportContactsExistingWorkPolicy](../-passport-contacts-existing-work-policy/index.md) = PassportContactsExistingWorkPolicy.KEEP, callback: [PassportContactsJavaWrapper.ScheduleCallback](-schedule-callback/index.md)): [Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md)<br>Try to schedule an periodic sync request. |
| [subscribeForWorkInfo](subscribe-for-work-info.md) | [passport-contacts]<br>fun [subscribeForWorkInfo](subscribe-for-work-info.md)(listener: [PassportContactsJavaWrapper.WorkInfoListener](-work-info-listener/index.md)): [Disposable](../../../../passport/passport/com.yandex.passport.common/-disposable/index.md)<br>Provides status of work done as listenable subscription. |
