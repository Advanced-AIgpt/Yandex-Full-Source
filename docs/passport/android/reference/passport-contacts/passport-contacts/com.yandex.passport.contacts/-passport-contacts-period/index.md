//[passport-contacts](../../../index.md)/[com.yandex.passport.contacts](../index.md)/[PassportContactsPeriod](index.md)

# PassportContactsPeriod

[passport-contacts]\
enum [PassportContactsPeriod](index.md) : Enum&lt;[PassportContactsPeriod](index.md)&gt; 

Defines period for [PassportContactsApi.schedulePeriodicSync](../-passport-contacts-api/schedule-periodic-sync.md).

## Entries

| | |
|---|---|
| [MONTHLY](-m-o-n-t-h-l-y/index.md) | [passport-contacts]<br>[MONTHLY](-m-o-n-t-h-l-y/index.md)(CommonTime(hours = 24 * 31))<br>Every 31 days. |
| [BIWEEKLY](-b-i-w-e-e-k-l-y/index.md) | [passport-contacts]<br>[BIWEEKLY](-b-i-w-e-e-k-l-y/index.md)(CommonTime(hours = 24 * 14))<br>Every 14 days. |
| [WEEKLY](-w-e-e-k-l-y/index.md) | [passport-contacts]<br>[WEEKLY](-w-e-e-k-l-y/index.md)(CommonTime(hours = 24 * 7))<br>Every 7 days. |
| [QAD](-q-a-d/index.md) | [passport-contacts]<br>[QAD](-q-a-d/index.md)(CommonTime(hours = 48))<br>Once per 2 days (48 hours). |
| [DAILY](-d-a-i-l-y/index.md) | [passport-contacts]<br>[DAILY](-d-a-i-l-y/index.md)(CommonTime(hours = 24))<br>Every 24 hours. |
| [BIDAILY](-b-i-d-a-i-l-y/index.md) | [passport-contacts]<br>[BIDAILY](-b-i-d-a-i-l-y/index.md)(CommonTime(hours = 12))<br>Every 12 hours. |
| [QID](-q-i-d/index.md) | [passport-contacts]<br>[QID](-q-i-d/index.md)(CommonTime(hours = 6))<br>Quater In Die (four times a day). Every 6 hours. |
| [TRIHOURLY](-t-r-i-h-o-u-r-l-y/index.md) | [passport-contacts]<br>[TRIHOURLY](-t-r-i-h-o-u-r-l-y/index.md)(CommonTime(hours = 3))<br>Every 3 hours. |
| [BIHOURLY](-b-i-h-o-u-r-l-y/index.md) | [passport-contacts]<br>[BIHOURLY](-b-i-h-o-u-r-l-y/index.md)(CommonTime(hours = 2))<br>Every 2 hours (120 minutes). |
| [HOURLY](-h-o-u-r-l-y/index.md) | [passport-contacts]<br>[HOURLY](-h-o-u-r-l-y/index.md)(CommonTime(hours = 1))<br>Every 60 minutes. |
| [HALF_HOURLY](-h-a-l-f_-h-o-u-r-l-y/index.md) | [passport-contacts]<br>[HALF_HOURLY](-h-a-l-f_-h-o-u-r-l-y/index.md)(CommonTime(minutes = 30))<br>Every 30 minutes. |
| [QUARTER_HOURLY](-q-u-a-r-t-e-r_-h-o-u-r-l-y/index.md) | [passport-contacts]<br>[QUARTER_HOURLY](-q-u-a-r-t-e-r_-h-o-u-r-l-y/index.md)(CommonTime(minutes = 15))<br>Every 15 minutes. |

## Properties

| Name | Summary |
|---|---|
| [time](time.md) | [passport-contacts]<br>val [time](time.md): CommonTime |
