# WebViewControllerAuthChallengeResolver

Протокол для обработки Authentication Challenge в методе делегате WKWebView

``` swift
public protocol WebViewControllerAuthChallengeResolver 
```

## Requirements

### process(challenge:​completion:​)

Обработать Authentication Challenge.

``` swift
func process(challenge: URLAuthenticationChallenge, completion: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void)
```

#### Parameters

  - challenge: authentication challenge.
  - completion: обработчик, который должен быть вызван, чтоб обработать челлендж.
