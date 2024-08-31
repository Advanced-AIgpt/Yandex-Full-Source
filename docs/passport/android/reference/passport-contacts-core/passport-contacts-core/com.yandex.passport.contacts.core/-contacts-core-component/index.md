//[passport-contacts-core](../../../index.md)/[com.yandex.passport.contacts.core](../index.md)/[ContactsCoreComponent](index.md)

# ContactsCoreComponent

[passport-contacts-core]\
@Singleton

@Component(modules = [[ContactsCoreModule::class](../../../passport-contacts-core/com.yandex.passport.contacts.core/index.md), [ContactsCoreDependencies::class](../-contacts-core-dependencies/index.md)])

interface [ContactsCoreComponent](index.md)

Core dagger2 component for contacts lib core. Use [Builder](-builder/index.md) to create. [ContactsCoreDependencies](../-contacts-core-dependencies/index.md) and application Context are required.

## Types

| Name | Summary |
|---|---|
| [Builder](-builder/index.md) | [passport-contacts-core]<br>@Component.Builder<br>interface [Builder](-builder/index.md) |

## Properties

| Name | Summary |
|---|---|
| [permissionFacade](permission-facade.md) | [passport-contacts-core]<br>abstract val [permissionFacade](permission-facade.md): [ContactPermissionFacade](../-contact-permission-facade/index.md) |
| [syncFacade](sync-facade.md) | [passport-contacts-core]<br>abstract val [syncFacade](sync-facade.md): [ContactSyncFacade](../-contact-sync-facade/index.md) |
