//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactPermissionFacade](index.md)

# ContactPermissionFacade

[passport-contacts-core]\
class [ContactPermissionFacade](index.md)@Injectconstructor(context: Context, permissionManager: PermissionManager, coroutineDispatchers: CoroutineDispatchers)

Facade class designed to check contact permissions before actual contacts sync request.

## Constructors

| | |
|---|---|
| [ContactPermissionFacade](-contact-permission-facade.md) | [passport-contacts-core]<br>@Inject<br>fun [ContactPermissionFacade](-contact-permission-facade.md)(context: Context, permissionManager: PermissionManager, coroutineDispatchers: CoroutineDispatchers) |

## Types

| Name | Summary |
|---|---|
| [Listener](-listener/index.md) | [passport-contacts-core]<br>fun interface [Listener](-listener/index.md)<br>A listener to call after permission is granted or denied. |

## Functions

| Name | Summary |
|---|---|
| [checkPermissions](check-permissions.md) | [passport-contacts-core]<br>suspend fun [checkPermissions](check-permissions.md)(activity: ComponentActivity, rationale: [ContactPermissionRationale](../-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn): Result&lt;PermissionStatus&gt;<br>Check (and request if needed) contact permissions in a coroutine. Coroutine is suspended until user grants or rejects permission.<br>[passport-contacts-core]<br>fun [checkPermissions](check-permissions.md)(activity: ComponentActivity, rationale: [ContactPermissionRationale](../-contact-permission-rationale/index.md) = ContactPermissionRationale.BuiltIn, listener: [ContactPermissionFacade.Listener](-listener/index.md)): Closeable<br>Check (and request if needed) contact permissions in a listener/subscription manner. Listener method is invoked when user grants or rejects permission. |
