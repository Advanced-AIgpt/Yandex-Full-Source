# PrebuiltUiFactory

Интерфейс фабрики для создания Prebuilt UI компонентов.

``` swift
public protocol PrebuiltUiFactory 
```

## Requirements

### createCardInputView(mode:​validationConfig:​)

Создать вью для ввода банковских карт.

``` swift
func createCardInputView(mode: CardInputMode, validationConfig: CardValidationConfig) -> CardInput
```

#### Parameters

  - mode: требуемый вид вью.
  - validationConfig: конфиг для валидации карт.

#### Returns

готовая вью.

### createCvnInputView()

Создать вью для ввода CVV/CVC кода.

``` swift
func createCvnInputView() -> CvnInput
```

#### Returns

готовая вью.

### createPaymentButtonView()

Создать вью основной кнопки.

``` swift
func createPaymentButtonView() -> PayButton
```

#### Returns

готовая вью.

### createSelectMethodTableView()

Создать таблицу для отображения списка методов.

``` swift
func createSelectMethodTableView() -> SelectMethodTableView
```

#### Returns

готовая вью таблицы.

### create3DSWebViewController(with:​delegate:​authChallengeResolver:​)

Создать вьюконтроллер для отображения 3ds-страницы.

``` swift
func create3DSWebViewController(with url: URL, delegate: WebViewControllerDelegate, authChallengeResolver: WebViewControllerAuthChallengeResolver?) -> PaymentSDKWebViewController
```

#### Parameters

  - url: урл 3ds-страницы.
  - delegate: делегат для вебвьюконтроллера.
  - authChallengeResolver: протокол для обработки Authentication Challenge в методе делегате WKWebView.

#### Returns

вьюконтроллер.

### createTinkoffWebViewController(with:​webViewDelegate:​authChallengeResolver:​tinkoffDelegate:​)

Создать вьюконтроллер для отображения страницы кредита Тинькофф.

``` swift
func createTinkoffWebViewController(with url: URL, webViewDelegate: WebViewControllerDelegate, authChallengeResolver: WebViewControllerAuthChallengeResolver?, tinkoffDelegate: TinkoffSubmitFormDelegate) -> PaymentSDKWebViewController
```

#### Parameters

  - url: урл страницы с кредитом.
  - webViewDelegate: делегат для вебвьюконтроллера.
  - authChallengeResolver: протокол для обработки Authentication Challenge в методе делегате WKWebView.
  - tinkoffDelegate: делегат для Тинькова.

#### Returns

вьюконтроллер.
