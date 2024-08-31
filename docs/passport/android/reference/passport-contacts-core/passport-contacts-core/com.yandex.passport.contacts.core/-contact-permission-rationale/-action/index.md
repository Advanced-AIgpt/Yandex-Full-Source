//[passport-contacts-core](../../../../index.md)/[com.yandex.passport.contacts.core](../../index.md)/[ContactPermissionRationale](../index.md)/[Action](index.md)

# Action

[passport-contacts-core]\
class [Action](index.md)(val action: suspend () -&gt; Boolean) : [ContactPermissionRationale](../index.md)

Provide custom rationale screen as suspendable coroutine. Coroutine should return boolean flag with true when user accepted rationale. Coroutine should suspend until rationale is hidden back with result.

## Constructors

| | |
|---|---|
| [Action](-action.md) | [passport-contacts-core]<br>fun [Action](-action.md)(action: suspend () -&gt; Boolean) |

## Properties

| Name | Summary |
|---|---|
| [action](action.md) | [passport-contacts-core]<br>val [action](action.md): suspend () -&gt; Boolean |
