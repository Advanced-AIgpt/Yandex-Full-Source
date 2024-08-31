# PaymentSDKWebViewController

Вебвьюконтроллер PaymentSDK. Используется для отображения страниц 3ds и кредитов Тинькова.

``` swift
public final class PaymentSDKWebViewController: UIViewController, WKNavigationDelegate, WKUIDelegate, WebViewControllerDelegate 
```

## Inheritance

`UIViewController`, `WKNavigationDelegate`, `WKUIDelegate`, [`WebViewControllerDelegate`](/WebViewControllerDelegate)

## Methods

### `viewDidLoad()`

``` swift
public override func viewDidLoad() 
```

### `viewDidLayoutSubviews()`

``` swift
public override func viewDidLayoutSubviews() 
```

### `webView(_:didReceive:completionHandler:)`

``` swift
public func webView(_ webView: WKWebView,
                 didReceive challenge: URLAuthenticationChallenge,
                 completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void)
```

### `webView(_:didFinish:)`

``` swift
public func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) 
```

### `webView(_:decidePolicyFor:decisionHandler:)`

``` swift
public func webView(_ webView: WKWebView,
                 decidePolicyFor navigationResponse: WKNavigationResponse,
                 decisionHandler: @escaping (WKNavigationResponsePolicy) -> Void) 
```

### `webView(_:createWebViewWith:for:windowFeatures:)`

``` swift
public func webView(_ webView: WKWebView,
                 createWebViewWith configuration: WKWebViewConfiguration,
                 for navigationAction: WKNavigationAction,
                 windowFeatures: WKWindowFeatures) -> WKWebView? 
```

### `webViewControllerDidCloseTap(_:)`

``` swift
public func webViewControllerDidCloseTap(_ controller: PaymentSDKWebViewController) 
```
