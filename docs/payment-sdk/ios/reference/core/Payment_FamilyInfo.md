# Payment.FamilyInfo

Информация о владельце карты, доступной пользователю.
Подробнее [по ссылке](https:​//wiki.yandex-team.ru/users/amosov-f/familypay/#variantformataotvetaruchkilpmvtraste)

``` swift
public struct FamilyInfo: Equatable 
```

## Inheritance

`Equatable`

## Properties

### `familyAdminUid`

Uid владельца карты.

``` swift
public let familyAdminUid: String
```

### `familyId`

Id семьи.

``` swift
public let familyId: String
```

### `expenses`

Текущее использование пользователем карты (в копейках).

``` swift
public let expenses: Int
```

### `limit`

Лимит карты, установленный родителем (в копейках).

``` swift
public let limit: Int
```

### `currency`

Валюта лимита, выбранная родителем (ISO 4217).

``` swift
public let currency: String
```

### `frame`

Тип лимита. Пр. "month", "day".

``` swift
public let frame: String
```

### `isUnlimited`

У карты отсутствует верхний лимит.

``` swift
public let isUnlimited: Bool
```
