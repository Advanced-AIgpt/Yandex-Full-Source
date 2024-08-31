//[passport](../../../../index.md)/[com.yandex.passport.api](../../index.md)/[PassportPersonProfile](../index.md)/[Builder](index.md)

# Builder

[passport]\
interface [Builder](index.md) : [PassportPersonProfile](../index.md)

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [build](build.md) | [passport]<br>abstract fun [build](build.md)(): [PassportPersonProfile](../index.md) |
| [clearBirthday](clear-birthday.md) | [passport]<br>abstract fun [clearBirthday](clear-birthday.md)(): [PassportPersonProfile.Builder](index.md) |
| [setBirthday](set-birthday.md) | [passport]<br>abstract fun [setBirthday](set-birthday.md)(@IntRange(from = 1900, to = 2100)year: Int, @IntRange(from = 1, to = 12)month: Int, @IntRange(from = 1, to = 31)dayOfMonth: Int): [PassportPersonProfile.Builder](index.md) |
| [setDisplayName](set-display-name.md) | [passport]<br>abstract fun [setDisplayName](set-display-name.md)(displayName: String?): [PassportPersonProfile.Builder](index.md) |
| [setFirstName](set-first-name.md) | [passport]<br>abstract fun [setFirstName](set-first-name.md)(firstName: String?): [PassportPersonProfile.Builder](index.md) |
| [setGender](set-gender.md) | [passport]<br>abstract fun [setGender](set-gender.md)(gender: [PassportPersonProfile.PassportGender](../-passport-gender/index.md)?): [PassportPersonProfile.Builder](index.md) |
| [setLastName](set-last-name.md) | [passport]<br>abstract fun [setLastName](set-last-name.md)(lastName: String?): [PassportPersonProfile.Builder](index.md) |

## Properties

| Name | Summary |
|---|---|
| [birthday](birthday.md) | [passport]<br>abstract override var [birthday](birthday.md): String? |
| [displayName](display-name.md) | [passport]<br>abstract override var [displayName](display-name.md): String? |
| [displayNames](display-names.md) | [passport]<br>abstract override var [displayNames](display-names.md): List&lt;String&gt;? |
| [firstName](first-name.md) | [passport]<br>abstract override var [firstName](first-name.md): String? |
| [gender](gender.md) | [passport]<br>abstract override var [gender](gender.md): [PassportPersonProfile.PassportGender](../-passport-gender/index.md)? |
| [lastName](last-name.md) | [passport]<br>abstract override var [lastName](last-name.md): String? |
