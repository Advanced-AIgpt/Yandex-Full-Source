# PaymentKitAuthExchanger

Протокол обмена скоупа токенов в Паспорте.

``` swift
public protocol PaymentKitAuthExchanger 
```

## Requirements

### exchangeOauthToken(uid:​originalOauthToken:​authEnvironment:​completion:​)

Обменять токен.

``` swift
func exchangeOauthToken(uid: String, originalOauthToken: String, authEnvironment: AuthEnvironment, completion: @escaping (ExternalResult<String>) -> Void)
```

#### Parameters

  - uid: uid пользователя.
  - originalOauthToken: оригинальный OAuth токен.
  - authEnvironment: окружение.
  - completion: комплишен с результатом
