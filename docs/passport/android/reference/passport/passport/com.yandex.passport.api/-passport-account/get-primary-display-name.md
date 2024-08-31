//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportAccount](index.md)/[getPrimaryDisplayName](get-primary-display-name.md)

# getPrimaryDisplayName

[passport]\

@NonNull

abstract fun [getPrimaryDisplayName](get-primary-display-name.md)(): String

Главное отображаемое имя пользователя. Пользователи (кроме некоторых категорий) могут свободно менять эту строку в настройках своего аккаунта в Паспорте. Удобно показывать эту строку под аватаркой пользователя. Можно использовать также [getSecondaryDisplayName](get-secondary-display-name.md) для показа двух строк, если позволяет дизайн.
