//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportUid](index.md)

# PassportUid

[passport]\
interface [PassportUid](index.md)

 Описывает паспортный uid - *уникальный* идентификатор, однозначно указывающий на определённый аккаунт в заданном паспортном окружении. 

 Является основным, рекомендуемым и **единственным правильным** способом идентификации аккаунта при работе с API библиотеки. Помимо библиотеки, **крайне рекомендуется** использовать uid в самом приложении при наличии любого контента, привязанного к конкретному аккаунту. То есть, необходимо уходить от использования системного name, каких либо логинов и/или email-адресов для такой идентификации. 

 Создать объект с uid-ом можно одним из двух фабричных методов: 

1. 
   [from](-factory/from.md) - требует только числовое значение uid-а, а окружение старается определить автоматически с помощью нехитрых эвристик;
2. 
   [from](-factory/from.md) - требует точного указания и окружения [PassportEnvironment](../-passport-environment/index.md), и значения uid-а.

 При сохранении uid-а в каком-либо хранилище, нужно как минимум записать его значение, возвращаемое из [getValue](get-value.md). Если вы работаете только в контексте боевого внешнего паспорта (см. [PASSPORT_ENVIRONMENT_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md)), то сохранять ещё и окружение uid-а из [getEnvironment](get-environment.md) необязательно. Тогда при последующей выгрузке сохранённого значения можно использовать фабричный метод [from](-factory/from.md). Если же вам нужно работать с несколькими окружениями, то желательно сохранить в хранилище и окружение из [getEnvironment](get-environment.md). Для этого нужно взять численное значение окружения с помощью [getInteger](../../../passport/com.yandex.passport.api/-passport-environment/integer.md) и сохранить его рядом со значеним uid-а. Конкретный способ хранения никак не регламентируется - как вам удобнее: 

- можно сохранить значение окружение в одной колонке таблицы sqlite, а значение uid-а - в другой;
- или объединить значения в одну строку через разделитель (сериализовать).

 После выгрузки обоих значений объект uid-а нужно создавать фабричным методом [from](-factory/from.md).

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportUid.Factory](-factory/from.md) |  |
| [com.yandex.passport.api.PassportEnvironment](../../../passport/com.yandex.passport.api/-passport-environment/integer.md) |  |
| [com.yandex.passport.api.Passport](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md) |  |

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>open class [Factory](-factory/index.md) |

## Functions

| Name | Summary |
|---|---|
| [getEnvironment](get-environment.md) | [passport]<br>@NonNull<br>abstract fun [getEnvironment](get-environment.md)(): [PassportEnvironment](../-passport-environment/index.md)<br>Возвращает ненулевый объект паспортного окружения [PassportEnvironment](../-passport-environment/index.md), которому принадлежит uid. |
| [getValue](get-value.md) | [passport]<br>abstract fun [getValue](get-value.md)(): Long<br>Возвращает числовое значение uid-а, однозначно идентифицирующее аккаунт в пределах заданного паспортного окружения. |
