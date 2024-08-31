//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportAnimationTheme](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportAnimationTheme](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportAnimationTheme](../index.md) |
| [setCloseBackEnterAnimation](set-close-back-enter-animation.md) | [passport]<br>abstract fun [setCloseBackEnterAnimation](set-close-back-enter-animation.md)(@AnimRescloseBackEnterAnimation: Int): [PassportAnimationTheme.Builder](index.md) |
| [setCloseBackExitAnimation](set-close-back-exit-animation.md) | [passport]<br>abstract fun [setCloseBackExitAnimation](set-close-back-exit-animation.md)(@AnimRescloseBackExitAnimation: Int): [PassportAnimationTheme.Builder](index.md) |
| [setCloseForwardEnterAnimation](set-close-forward-enter-animation.md) | [passport]<br>abstract fun [setCloseForwardEnterAnimation](set-close-forward-enter-animation.md)(@AnimRescloseForwardEnterAnimation: Int): [PassportAnimationTheme.Builder](index.md) |
| [setCloseForwardExitAnimation](set-close-forward-exit-animation.md) | [passport]<br>abstract fun [setCloseForwardExitAnimation](set-close-forward-exit-animation.md)(@AnimRescloseForwardExitAnimation: Int): [PassportAnimationTheme.Builder](index.md) |
| [setOpenEnterAnimation](set-open-enter-animation.md) | [passport]<br>abstract fun [setOpenEnterAnimation](set-open-enter-animation.md)(@AnimResopenEnterAnimation: Int): [PassportAnimationTheme.Builder](index.md) |
| [setOpenExitAnimation](set-open-exit-animation.md) | [passport]<br>abstract fun [setOpenExitAnimation](set-open-exit-animation.md)(@AnimResopenExitAnimation: Int): [PassportAnimationTheme.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [closeBackEnterAnimation](close-back-enter-animation.md) | [passport]<br>@get:AnimRes<br>@set:AnimRes<br>abstract override var [closeBackEnterAnimation](close-back-enter-animation.md): Int<br>Анимация при закрытии активити паспорта (назад по флоу) |
| [closeBackExitAnimation](close-back-exit-animation.md) | [passport]<br>@get:AnimRes<br>@set:AnimRes<br>abstract override var [closeBackExitAnimation](close-back-exit-animation.md): Int<br>Анимация отображения следующей активити при закрытии паспорта (назад по флоу) |
| [closeForwardEnterAnimation](close-forward-enter-animation.md) | [passport]<br>@get:AnimRes<br>@set:AnimRes<br>abstract override var [closeForwardEnterAnimation](close-forward-enter-animation.md): Int<br>Анимация при закрытии активити паспорта (вперед по флоу) |
| [closeForwardExitAnimation](close-forward-exit-animation.md) | [passport]<br>@get:AnimRes<br>@set:AnimRes<br>abstract override var [closeForwardExitAnimation](close-forward-exit-animation.md): Int<br>Анимация отображения следующей активити при закрытии паспорта (вперед по флоу) |
| [openEnterAnimation](open-enter-animation.md) | [passport]<br>@get:AnimRes<br>@set:AnimRes<br>abstract override var [openEnterAnimation](open-enter-animation.md): Int<br>Анимация при открытии активити паспорта |
| [openExitAnimation](open-exit-animation.md) | [passport]<br>@get:AnimRes<br>@set:AnimRes<br>abstract override var [openExitAnimation](open-exit-animation.md): Int<br>Анимация скрытия предыдущей активити при открытии паспорта |
