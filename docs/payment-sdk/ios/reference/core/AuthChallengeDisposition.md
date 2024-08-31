# AuthChallengeDisposition

Типы проверок при установке соединения в методах делегатов URLSession и WKWebView
urlSession(*:​, didReceive challenge:​completionHandler:​)
webView(*:​, didReceive challenge:​ completionHandler:​)

``` swift
public enum AuthChallengeDisposition 
```

## Enumeration Cases

### `` `default` ``

Дефолтная проверка сертификата(.performDefaultHandling)

``` swift
case `default`
```

### `allowEverything`

Разрешать любое соединение

``` swift
case allowEverything
```

### `sslPinning`

Стандартный SSLPinning

``` swift
case sslPinning
```

### `allowedRevokedYandexCAWithExtraCertificates`

Игнорировать ошибку Revoked Certificate для YandexCA и переданных extra certificates

``` swift
case allowedRevokedYandexCAWithExtraCertificates(certificates: [SecCertificate])
```
