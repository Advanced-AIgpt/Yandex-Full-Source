//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsWorkStatus](index.md)

# PassportContactsWorkStatus

[passport-contacts]\
enum [PassportContactsWorkStatus](index.md) : Enum&lt;[PassportContactsWorkStatus](index.md)&gt;

## See also

passport-contacts

| | |
|---|---|
| androidx.work.WorkInfo.State |  |

## Entries

| | |
|---|---|
| [CANCELLED](-c-a-n-c-e-l-l-e-d/index.md) | [passport-contacts]<br>[CANCELLED](-c-a-n-c-e-l-l-e-d/index.md)()<br>Used to indicate that the WorkRequest has been cancelled and will not execute. All dependent work will also be marked as #CANCELLED and will not run. |
| [BLOCKED](-b-l-o-c-k-e-d/index.md) | [passport-contacts]<br>[BLOCKED](-b-l-o-c-k-e-d/index.md)()<br>Used to indicate that the WorkRequest is currently blocked because its prerequisites haven't finished successfully. |
| [FAILED](-f-a-i-l-e-d/index.md) | [passport-contacts]<br>[FAILED](-f-a-i-l-e-d/index.md)()<br>Used to indicate that the WorkRequest has completed in a failure state.  All dependent work will also be marked as #FAILED and will never run. |
| [SUCCEEDED](-s-u-c-c-e-e-d-e-d/index.md) | [passport-contacts]<br>[SUCCEEDED](-s-u-c-c-e-e-d-e-d/index.md)()<br>Used to indicate that the WorkRequest has completed in a successful state.  Note that PeriodicWorkRequest will never enter this state (they will simply go back to .ENQUEUED and be eligible to run again). |
| [RUNNING](-r-u-n-n-i-n-g/index.md) | [passport-contacts]<br>[RUNNING](-r-u-n-n-i-n-g/index.md)()<br>Used to indicate that the WorkRequest is currently being executed. |
| [ENQUEUED](-e-n-q-u-e-u-e-d/index.md) | [passport-contacts]<br>[ENQUEUED](-e-n-q-u-e-u-e-d/index.md)()<br>Used to indicate that the WorkRequest is enqueued and eligible to run when its Constraints are met and resources are available. |

## Properties

| Name | Summary |
|---|---|
| [isFinished](is-finished.md) | [passport-contacts]<br>val [isFinished](is-finished.md): Boolean<br>Returns true if this State is considered finished. |
