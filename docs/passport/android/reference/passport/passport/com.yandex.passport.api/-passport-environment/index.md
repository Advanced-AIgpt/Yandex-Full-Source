//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[PassportEnvironment](index.md)

# PassportEnvironment

[passport]\
interface [PassportEnvironment](index.md)

Описывает паспортное окружение.

На данный момент библиотека поддерживает несколько различных окружений, каждому из которых соответствует заранее проинициализированный статичный объект. Получить их можно двумя способами:

1. 

В run-time через фабричный метод [PassportEnvironment.Factory.from](-factory/from.md), принимающий уникальное число, которое соответствует известному окружению (см. также .getInteger);

1. 

В compile-time через переменные PASSPORT_ENVIRONMENT_* в [Passport](../-passport/index.md):

- 
   [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md) - боевой внешний паспорт, условный [passport.yandex.ru](https://passport.yandex.ru);
- 
   [Passport.PASSPORT_ENVIRONMENT_TEAM_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-p-r-o-d-u-c-t-i-o-n.md) - боевой внутренний паспорт, [passport.yandex-team.ru](https://passport.yandex-team.ru);
- 
   [Passport.PASSPORT_ENVIRONMENT_TESTING](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-s-t-i-n-g.md) - тестовый внешний паспорт, [passport-test.yandex.ru](https://passport-test.yandex.ru);
- 
   [Passport.PASSPORT_ENVIRONMENT_TEAM_TESTING](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-t-e-a-m_-t-e-s-t-i-n-g.md) - тестовый внутренний паспорт, [passport-test.yandex-team.ru](https://passport-test.yandex-team.ru);
- 
   [Passport.PASSPORT_ENVIRONMENT_RC](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-r-c.md) - релиз-кандидат внешнего паспорта, [passport-rc.yandex.ru](https://passport-rc.yandex.ru).

Обычно приложения знать не знают ни про какие дополнительные окружения и работают только в контексте боевого внешнего окружения. То есть в большинстве ситуаций его можно считать дефолтным, а при необходимости явного указания (например, при создании объекта [PassportUid](../-passport-uid/index.md), если у вас в наличии только числовое значение uid, но вы точно знаете, что оно от боевого внешнего окружения) можно использовать [Passport.PASSPORT_ENVIRONMENT_PRODUCTION](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-p-r-o-d-u-c-t-i-o-n.md).

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportEnvironment.Factory](-factory/from.md) |  |
| [com.yandex.passport.api.Passport](../-passport/-p-a-s-s-p-o-r-t_-e-n-v-i-r-o-n-m-e-n-t_-r-c.md) |  |
| [com.yandex.passport.api.PassportUid](../-passport-uid/index.md) |  |

## Types

| Name | Summary |
|---|---|
| [Factory](-factory/index.md) | [passport]<br>object [Factory](-factory/index.md) |

## Properties

| Name | Summary |
|---|---|
| [integer](integer.md) | [passport]<br>abstract val [integer](integer.md): Int<br>Возвращает уникальное и постоянное число, однозначно идентифицирующее это паспортное окружение. |

## Inheritors

| Name |
|---|
| [KPassportEnvironment](../-k-passport-environment/index.md) |
