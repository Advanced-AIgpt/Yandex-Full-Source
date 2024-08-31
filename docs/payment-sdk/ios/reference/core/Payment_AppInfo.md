# Payment.AppInfo

Информация о турбоаппе ПП/Браузера.

``` swift
public struct AppInfo 
```

## Initializers

### `init()`

``` swift
public init() 
```

## Properties

### `psuid`

``` swift
public var psuid: String?
```

### `tsid`

``` swift
public var tsid: String?
```

### `appid`

``` swift
public var appid: String?
```

## Methods

### `build(psuid:tsid:appid:)`

``` swift
public static func build(psuid: String?, tsid: String?, appid: String?) -> AppInfo 
```
