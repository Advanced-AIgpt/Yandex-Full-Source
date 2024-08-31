//[passport-contacts](../../index.md)/[com.yandex.passport.contacts](index.md)

# Package com.yandex.passport.contacts

## Types

| Name | Summary |
|---|---|
| [PassportContacts](-passport-contacts/index.md) | [passport-contacts]<br>object [PassportContacts](-passport-contacts/index.md)<br>Main entry point for passport's contact synchronization feature. |
| [PassportContactsApi](-passport-contacts-api/index.md) | [passport-contacts]<br>interface [PassportContactsApi](-passport-contacts-api/index.md)<br>Provides api to enqueue synchronization and get status work flow. |
| [PassportContactsConfiguration](-passport-contacts-configuration/index.md) | [passport-contacts]<br>data class [PassportContactsConfiguration](-passport-contacts-configuration/index.md)(val context: Context, val useRemoteWorkManager: Boolean = false, val coroutineDispatchers: CoroutineDispatchers = CoroutineDispatchersImpl())<br>Used to configure an api from host. |
| [PassportContactsExistingWorkPolicy](-passport-contacts-existing-work-policy/index.md) | [passport-contacts]<br>enum [PassportContactsExistingWorkPolicy](-passport-contacts-existing-work-policy/index.md) : Enum&lt;[PassportContactsExistingWorkPolicy](-passport-contacts-existing-work-policy/index.md)&gt; <br>Defines how to behave when sync work was already scheduled before. |
| [PassportContactsJavaWrapper](-passport-contacts-java-wrapper/index.md) | [passport-contacts]<br>class [PassportContactsJavaWrapper](-passport-contacts-java-wrapper/index.md)<br>Java-wrapper for [PassportContactsApi](-passport-contacts-api/index.md). |
| [PassportContactsNoAccountException](-passport-contacts-no-account-exception/index.md) | [passport-contacts]<br>class [PassportContactsNoAccountException](-passport-contacts-no-account-exception/index.md) : Exception |
| [PassportContactsPeriod](-passport-contacts-period/index.md) | [passport-contacts]<br>enum [PassportContactsPeriod](-passport-contacts-period/index.md) : Enum&lt;[PassportContactsPeriod](-passport-contacts-period/index.md)&gt; <br>Defines period for [PassportContactsApi.schedulePeriodicSync](-passport-contacts-api/schedule-periodic-sync.md). |
| [PassportContactsRemoteWorkerService](-passport-contacts-remote-worker-service/index.md) | [passport-contacts]<br>class [PassportContactsRemoteWorkerService](-passport-contacts-remote-worker-service/index.md) : RemoteWorkerService |
| [PassportContactsScheduleStatus](-passport-contacts-schedule-status/index.md) | [passport-contacts]<br>enum [PassportContactsScheduleStatus](-passport-contacts-schedule-status/index.md) : Enum&lt;[PassportContactsScheduleStatus](-passport-contacts-schedule-status/index.md)&gt; <br>Return status from schedule* methods. Used to get a result feedback of schedule attempt. |
| [PassportContactsWorkInfo](-passport-contacts-work-info/index.md) | [passport-contacts]<br>data class [PassportContactsWorkInfo](-passport-contacts-work-info/index.md)(val status: [PassportContactsWorkStatus](-passport-contacts-work-status/index.md), val attemptCount: Int)<br>Provides info about scheduled work. |
| [PassportContactsWorkStatus](-passport-contacts-work-status/index.md) | [passport-contacts]<br>enum [PassportContactsWorkStatus](-passport-contacts-work-status/index.md) : Enum&lt;[PassportContactsWorkStatus](-passport-contacts-work-status/index.md)&gt; |
