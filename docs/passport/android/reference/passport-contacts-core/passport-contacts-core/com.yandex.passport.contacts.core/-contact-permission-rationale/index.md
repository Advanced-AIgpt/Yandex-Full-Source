//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactPermissionRationale](index.md)

# ContactPermissionRationale

[passport-contacts-core]\
interface [ContactPermissionRationale](index.md)

Configuration for rationale to show before requesting a permission.

## Types

| Name | Summary |
|---|---|
| [Action](-action/index.md) | [passport-contacts-core]<br>class [Action](-action/index.md)(val action: suspend () -&gt; Boolean) : [ContactPermissionRationale](index.md)<br>Provide custom rationale screen as suspendable coroutine. Coroutine should return boolean flag with true when user accepted rationale. Coroutine should suspend until rationale is hidden back with result. |
| [Alert](-alert/index.md) | [passport-contacts-core]<br>data class [Alert](-alert/index.md)(val alertInit: AlertBuilder.((Boolean) -&gt; Unit) -&gt; Unit) : [ContactPermissionRationale](index.md)<br>Provide custom rationale as android.app.AlertDialog. |
| [BuiltIn](-built-in/index.md) | [passport-contacts-core]<br>object [BuiltIn](-built-in/index.md) : [ContactPermissionRationale](index.md)<br>Use built-in rationale, bundled with the contacts lib. (Recommended) |
| [Div](-div/index.md) | [passport-contacts-core]<br>data class [Div](-div/index.md)(val divData: String) : [ContactPermissionRationale](index.md)<br>Provide custom rationale screen as div json data. |
| [None](-none/index.md) | [passport-contacts-core]<br>object [None](-none/index.md) : [ContactPermissionRationale](index.md)<br>No actual rationale will be shown. Host itself is responsible for explaining the user why permission is requested. |

## Inheritors

| Name |
|---|
| [None](-none/index.md) |
| [BuiltIn](-built-in/index.md) |
| [Div](-div/index.md) |
| [Action](-action/index.md) |
| [Alert](-alert/index.md) |
