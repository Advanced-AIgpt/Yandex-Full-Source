//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportVisualProperties](index.md)/[isNoReturnToHost](is-no-return-to-host.md)

# isNoReturnToHost

[passport]\
abstract val [isNoReturnToHost](is-no-return-to-host.md): Boolean

Disable possibilities to return back to host when auth is shown.

- 
   When active accounts present: fullscreen selection will be shown instead of bottom sheet.
- 
   When no active accounts: back button will be not shown on auth screen.

Should used when the authorization window is the starting one in your application (e.g. Yandex.Disk).
