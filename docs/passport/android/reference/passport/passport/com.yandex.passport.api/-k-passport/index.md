//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassport](index.md)

# KPassport

[passport]\
object [KPassport](index.md)

Experimental replacement for [Passport](../-passport/index.md) as central entry point of android passport library. It is designed to be Kotlin-compatible.

Not designed to use from java: use [Passport](../-passport/index.md) for oldtimer java clients.

## Functions

| Name | Summary |
|---|---|
| [createPassportApi](create-passport-api.md) | [passport]<br>fun [createPassportApi](create-passport-api.md)(context: Context): [KPassportApi](../-k-passport-api/index.md)<br>Returns the object to be used to call the [KPassportApi](../-k-passport-api/index.md) methods. It can be called several times, but this behavior is not recommended. All checks will be performed every time the method is called. |
| [initialize](initialize.md) | [passport]<br>fun [initialize](initialize.md)(context: Context, init: [PassportProperties.Builder](../-passport-properties/-builder/index.md).() -&gt; Unit)<br>Initializing the API for the *:passport* process in Application.onCreate. Be sure to call this method before accessing the library. |
| [killPassportProcess](kill-passport-process.md) | [passport]<br>fun [killPassportProcess](kill-passport-process.md)(context: Context)<br>Kills the *:passport* process from the client process. It can be used to restart the *:passport* process with other [PassportProperties](../-passport-properties/index.md). |

## Properties

| Name | Summary |
|---|---|
| [isInPassportProcess](is-in-passport-process.md) | [passport]<br>val [isInPassportProcess](is-in-passport-process.md): Boolean<br>Returns *true* if the current process is *:passport*, otherwise - *false*. Initialized in onCreate of the library's internal content provider, which runs in the *:passport* process. |
