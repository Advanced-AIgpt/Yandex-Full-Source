//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsScheduleStatus](index.md)

# PassportContactsScheduleStatus

[passport-contacts]\
enum [PassportContactsScheduleStatus](index.md) : Enum&lt;[PassportContactsScheduleStatus](index.md)&gt; 

Return status from schedule* methods. Used to get a result feedback of schedule attempt.

## Entries

| | |
|---|---|
| [REQUEST_CONFIRMED](-r-e-q-u-e-s-t_-c-o-n-f-i-r-m-e-d/index.md) | [passport-contacts]<br>[REQUEST_CONFIRMED](-r-e-q-u-e-s-t_-c-o-n-f-i-r-m-e-d/index.md)(true)<br>Request was succeeded. |
| [NO_USABLE_ACCOUNTS](-n-o_-u-s-a-b-l-e_-a-c-c-o-u-n-t-s/index.md) | [passport-contacts]<br>[NO_USABLE_ACCOUNTS](-n-o_-u-s-a-b-l-e_-a-c-c-o-u-n-t-s/index.md)(false)<br>No usable passport accounts was found for synchronization. Request was failed. |
| [NO_PERMISSION](-n-o_-p-e-r-m-i-s-s-i-o-n/index.md) | [passport-contacts]<br>[NO_PERMISSION](-n-o_-p-e-r-m-i-s-s-i-o-n/index.md)(false)<br>Contact permission was not granted by user. Request was failed. |

## Properties

| Name | Summary |
|---|---|
| [isSuccess](is-success.md) | [passport-contacts]<br>val [isSuccess](is-success.md): Boolean<br>true here if request was scheduled successfully. |
