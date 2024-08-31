//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[KPassportEnvironment](index.md)/[integer](integer.md)

# integer

[passport]\
open override val [integer](integer.md): Int

Возвращает уникальное и постоянное число, однозначно идентифицирующее это паспортное окружение.

Это значение можно сохранять в персистентном хранилище, а при поддержке приложением нескольких окружений (например, при одновременной работе и с внешним, и с внутренним паспортом) - настоятельно рекомендуется. Загруженное из хранилища число можно использовать как параметр фабричного метода [PassportEnvironment.Factory.from](../-passport-environment/-factory/from.md) для получения соответствующего ему объекта окружения.

#### Return

Уникальное число, идентифицирующее известное паспортное окружение.

## See also

passport

| | |
|---|---|
| [com.yandex.passport.api.PassportEnvironment.Factory](../-passport-environment/-factory/from.md) |  |
