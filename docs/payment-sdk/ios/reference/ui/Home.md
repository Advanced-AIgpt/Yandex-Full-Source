# Types

  - [RadioButtonCell](RadioButtonCell.md):
    Ячейка с радиобаттоном, родительская для всех дефолтных ячеек таблицы.
  - [SelectMethodTableView](SelectMethodTableView.md):
    Таблица для отображения списка методов оплат. Может быть расширена и отображать кастомные данные вместе с методами.
  - [CardBinValidationConfig.Builder](CardBinValidationConfig_Builder.md)
  - [CardExpirationDateValidationConfig.Builder](CardExpirationDateValidationConfig_Builder.md)
  - [CommandWith](CommandWith.md):
    Воспомогательный класс для хранения коллбека.
  - [Command](Command.md):
    Вспомогательный класс для хранения коллбека без параметров.
  - [PayButton](PayButton.md):
    Класс основной кнопки в PaymentSDK
  - [PaymentSDKWebViewController](PaymentSDKWebViewController.md):
    Вебвьюконтроллер PaymentSDK. Используется для отображения страниц 3ds и кредитов Тинькова.
  - [CardInputState](CardInputState.md):
    Состояния вью.
  - [CardInputMode](CardInputMode.md):
    Возможные виды вью для ввода данных банковских карт.
  - [PaymentSdkCellProps.CellType](PaymentSdkCellProps_CellType.md):
    Типы ячеек.
  - [PaymentCellIconsMode](PaymentCellIconsMode.md):
    Режим отображения иконок в ячейках с методами оплаты.
  - [Theme](Theme.md):
    Тема для UI.
  - [Theme.Kind](Theme_Kind.md):
    Типы тем.
  - [ActionButtonProps.Style](ActionButtonProps_Style.md):
    Варианты отрисовки кнопки.
  - [TinkoffFormState](TinkoffFormState.md):
    Статусы от страницы кредитов Тинькова.
  - [PaymentSdkCellProps](PaymentSdkCellProps.md):
    Ячейки для отображения методов из PaymentSDK&
  - [SelectMethodProps](SelectMethodProps.md):
    Свойства таблицы.
  - [Theme.Builder](Theme_Builder.md):
    Билдер для темы.
  - [CardBinValidationConfig](CardBinValidationConfig.md):
    Конфиг для валидации BIN номеров банковских карт.
  - [CardExpirationDateValidationConfig](CardExpirationDateValidationConfig.md):
    Конфиг для валидации expiration date банковских карт.
  - [CardValidationConfig](CardValidationConfig.md):
    Общий конфиг валидации карт.
  - [ActionButtonProps](ActionButtonProps.md):
    Свойства кнопки.
  - [ActionButtonProps.Labels](ActionButtonProps_Labels.md):
    Подписи с суммами.

# Protocols

  - [CardInput](CardInput.md):
    Протокол для работы с логикой ввода данных банковских карт.
  - [CvnInput](CvnInput.md):
    Протокол для работы с логикой ввода CVC/CVV.
  - [PrebuiltUiFactory](PrebuiltUiFactory.md):
    Интерфейс фабрики для создания Prebuilt UI компонентов.
  - [SelectCellProps](SelectCellProps.md):
    Общий протокол для всех возможных свойств ячеек таблицы методов.
  - [CustomCellProvider](CustomCellProvider.md):
    Протокол провайдера кастомных ячеек в таблицу методов.
  - [WebViewControllerDelegate](WebViewControllerDelegate.md):
    Делегат вебвьюконтроллера.
  - [WebViewControllerAuthChallengeResolver](WebViewControllerAuthChallengeResolver.md):
    Протокол для обработки Authentication Challenge в методе делегате WKWebView
  - [TinkoffSubmitFormDelegate](TinkoffSubmitFormDelegate.md):
    Протокол обработки состояния страницы с кредитами.

# Global Functions

  - [createPrebuiltUiFactory(theme:​)](createPrebuiltUiFactory\(theme_\).md):
    Создать фабрику для Prebuilt UI компонентов.

# Extensions

  - [CardExpirationDateField](CardExpirationDateField.md)
