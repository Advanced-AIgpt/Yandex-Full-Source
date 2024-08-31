# Types

  - [BindCardMediator](BindCardMediator.md):
    Медиатор для привязки карт.
  - [CardInputMediator](CardInputMediator.md):
    Общий медиатор для работы со вью ввода банковских карт. Содержит в себе базовую логику работы с состояниями вью.
  - [NewCardPayMediator](NewCardPayMediator.md):
    Медиатор для оплаты новой картой. Дополнительно проверяет, что пользовательский email не пуст -
    это может быть полезно для анонимных платежей.
  - [SimplePaymentMediator](SimplePaymentMediator.md):
    Простой медиатор для процесса оплаты. Управляет состоянием кнопки оплаты, экрана и отображением 3ds, и может сам вызывать передачу кода CVC/CVV когда нужно.
  - [CardScreenState](CardScreenState.md):
    Состояние экрана с вводом данных карт.
  - [CardActionButtonTitle](CardActionButtonTitle.md):
    Тип надписи на основной кнопке экрана ввода данных.
  - [CardActionButtonState](CardActionButtonState.md):
    Возможные состояния основной кнопки экрана ввода данных карт.
  - [PaymentScreenState](PaymentScreenState.md):
    Возможные состояния экрана оплаты.

# Protocols

  - [BindProcessingApi](BindProcessingApi.md):
    Протокол для привязки карт.
  - [PaymentProcessingApi](PaymentProcessingApi.md):
    Протокол для оплаты.
  - [WebView3dsDelegate](WebView3dsDelegate.md):
    Делегат для отображения 3ds.
  - [CardScreenDelegate](CardScreenDelegate.md):
    Протокол для делегата экана ввода данных карт.
  - [CardActionButton](CardActionButton.md):
    Протокол основной кнопки действия экрана ввода данных карт.
  - [PaymentScreenDelegate](PaymentScreenDelegate.md)
