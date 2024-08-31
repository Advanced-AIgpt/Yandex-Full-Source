//[passport](../../../index.md)/[com.yandex.passport.api](../index.md)/[Passport](index.md)/[createPassportApi](create-passport-api.md)

# createPassportApi

[passport]\

@NonNull

open fun [createPassportApi](create-passport-api.md)(@NonNullcontext: Context): [PassportApi](../-passport-api/index.md)

Возвращает объект, который нужно использовать для вызова методов [PassportApi](../-passport-api/index.md). Предназначен для вызова из клиентского процесса. Может быть вызван несколько раз, но такое поведение не рекомендуется. Все проверки будут выполнены при каждом вызове метода

#### Return

[PassportApi](../-passport-api/index.md)

## Parameters

passport

| | |
|---|---|
| context | Context |
