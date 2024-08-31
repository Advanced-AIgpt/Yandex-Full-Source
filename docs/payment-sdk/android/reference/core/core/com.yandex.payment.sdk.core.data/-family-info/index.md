//[core](../../../index.md)/[com.yandex.payment.sdk.core.data](../index.md)/[FamilyInfo](index.md)

# FamilyInfo

[core]\
class [FamilyInfo](index.md)(**familyAdminUid**: String, **familyId**: String, **expenses**: Int, **limit**: Int, **currency**: String, **frame**: String, **isUnlimited**: Boolean) : Parcelable

Информация о владельце карты, доступной пользователю Подробнее [по ссылке](https://wiki.yandex-team.ru/users/amosov-f/familypay/#variantformataotvetaruchkilpmvtraste)

## Constructors

| | |
|---|---|
| [FamilyInfo](-family-info.md) | [core]<br>fun [FamilyInfo](-family-info.md)(familyAdminUid: String, familyId: String, expenses: Int, limit: Int, currency: String, frame: String, isUnlimited: Boolean) |

## Properties

| Name | Summary |
|---|---|
| [available](available.md) | [core]<br>val [available](available.md): Double |
| [currency](currency.md) | [core]<br>val [currency](currency.md): String<br>валюта лимита, выбранная родителем (ISO 4217) |
| [expenses](expenses.md) | [core]<br>val [expenses](expenses.md): Int<br>текущее использование пользователем карты (в копейках) |
| [familyAdminUid](family-admin-uid.md) | [core]<br>val [familyAdminUid](family-admin-uid.md): String<br>uid владельца карты |
| [familyId](family-id.md) | [core]<br>val [familyId](family-id.md): String<br>id семьи |
| [frame](frame.md) | [core]<br>val [frame](frame.md): String<br>тип лимита. |
| [isUnlimited](is-unlimited.md) | [core]<br>val [isUnlimited](is-unlimited.md): Boolean<br>У карты отсутствует верхний лимит |
| [limit](limit.md) | [core]<br>val [limit](limit.md): Int<br>лимит карты, установленный родителем (в копейках) |
