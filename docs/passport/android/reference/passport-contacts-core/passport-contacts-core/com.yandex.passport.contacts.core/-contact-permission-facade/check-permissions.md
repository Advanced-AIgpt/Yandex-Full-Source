//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactPermissionFacade](index.md)/[checkPermissions](check-permissions.md)

# checkPermissions

[passport-contacts-core]\
suspend fun [checkPermissions](check-permissions.md)(activity: ComponentActivity, rationale: [ContactPermissionRationale](../-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn): Result&lt;PermissionStatus&gt;

Check (and request if needed) contact permissions in a coroutine. Coroutine is suspended until user grants or rejects permission.

#### Return

status of permission after request is finished

## Parameters

passport-contacts-core

| | |
|---|---|
| activity | ComponentActivity instance to serve as host for permission request and showing rationale |
| rationale | to show for user when asking for permission |

[passport-contacts-core]\
fun [checkPermissions](check-permissions.md)(activity: ComponentActivity, rationale: [ContactPermissionRationale](../-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, listener: [ContactPermissionFacade.Listener](-listener/index.md)): Closeable

Check (and request if needed) contact permissions in a listener/subscription manner. Listener method is invoked when user grants or rejects permission.

#### Return

a Closeable to close when permision request in not valid anymore (e.g. activity is destroyed)

## Parameters

passport-contacts-core

| | |
|---|---|
| activity | ComponentActivity instance to serve as host for permission request and showing rationale |
| rationale | to show for user when asking for permission |
| listener | to call after permission is granted or denied |
