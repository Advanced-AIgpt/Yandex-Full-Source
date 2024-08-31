# BindApi

Интерфейс для работы с банковскими картами.

``` swift
public protocol BindApi 
```

## Requirements

### bindCardWithVerify(completion:​)

Привязать банковскую карточку для пользователя.
Используется API V2 (то есть привязка со вводом 3DS)
**Важно:​** обязательно должен быть задан валидный сервис токен в `Merchant`.

``` swift
func bindCardWithVerify(completion: @escaping (PaymentKitResult<BoundCard>) -> Void)
```

#### Parameters

  - completion: коллбек с данными о привязке в случае успеха.

### bindCardWithoutVerify(completion:​)

Привязать банковскую карточку для пользователя.
Используется API V1 (то есть привязка без ввода 3DS кода).
**Не рекомендуется** к использованию.

``` swift
func bindCardWithoutVerify(completion: @escaping (PaymentKitResult<BoundCard>) -> Void)
```

#### Parameters

  - completion: коллбек с данными о привязке в случае успеха.

### unbindCard(cardId:​completion:​)

Отвязать карту.

``` swift
func unbindCard(cardId: Payment.Card.Id, completion: @escaping (PaymentKitResult<Void>) -> Void)
```

#### Parameters

  - cardId: идентификатор карты.
  - completion: коллбек с результатом.

### verifyCard(cardId:​completion:​)

Выполнить верификационный платёж по карте. Поддерживается только верификация через 3ds.

``` swift
func verifyCard(cardId: Payment.Card.Id, completion: @escaping (PaymentKitResult<BoundCard>) -> Void)
```

#### Parameters

  - cardId: идентификатор карты.
  - completion: коллбек с результатом.

### cancel()

Отменить процесс привязки или верификации. **Важно:​** будет отменен только поллинг, сама карта может успеть привязаться/верифицироваться, если пользователь прошел все челленджи.

``` swift
func cancel()
```
