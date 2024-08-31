//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[Analytics](index.md)

# Analytics

[passport-contacts-core]\
@AnyThread

interface [Analytics](index.md)

Interface, used to report some analytics events. Push them to Metrica if you want.

## Functions

| Name | Summary |
|---|---|
| [reportEvent](report-event.md) | [passport-contacts-core]<br>abstract fun [reportEvent](report-event.md)(event: String)<br>Report an event without params.<br>[passport-contacts-core]<br>abstract fun [reportEvent](report-event.md)(event: String, params: Map&lt;String, Any?&gt;)<br>Report an event.<br>[passport-contacts-core]<br>abstract fun [reportEvent](report-event.md)(event: String, arg0: String, value0: Any?)<br>Report an event with one named parameter<br>[passport-contacts-core]<br>abstract fun [reportEvent](report-event.md)(event: String, arg0: String, value0: Any?, arg1: String, value1: Any?)<br>Report an event with two named parameters |
