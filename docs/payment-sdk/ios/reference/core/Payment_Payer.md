# Payment.Payer

Информация о пользователе.

``` swift
public struct Payer: Equatable 
```

## Inheritance

`Equatable`

## Initializers

### `init(oauthToken:email:uid:)`

``` swift
public init(oauthToken: String?, email: String?, uid: String?) 
```

## Properties

### `oauthToken`

Паспортный OAuth-токен пользователя.

``` swift
public let oauthToken: String?
```

### `email`

Почта.

``` swift
public let email: String?
```

### `uid`

Uid пользователя.

``` swift
public let uid: String?
```
