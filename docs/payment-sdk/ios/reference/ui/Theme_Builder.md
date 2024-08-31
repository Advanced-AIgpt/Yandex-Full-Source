# Theme.Builder

Билдер для темы.

``` swift
public struct Builder 
```

## Initializers

### `init(basedOn:)`

``` swift
public init(basedOn kind: Kind) 
```

## Methods

### `withPrimaryTintColor(_:)`

``` swift
public func withPrimaryTintColor(_ color: UIColor) -> Builder 
```

### `withSecondaryTintColor(_:)`

``` swift
public func withSecondaryTintColor(_ color: UIColor) -> Builder 
```

### `withActivityIndicatorColor(_:)`

``` swift
public func withActivityIndicatorColor(_ color: UIColor) -> Builder 
```

### `withPaymentNormalTextColor(_:)`

``` swift
public func withPaymentNormalTextColor(_ color: UIColor) -> Builder 
```

### `withPaymentDisabledTextColor(_:)`

``` swift
public func withPaymentDisabledTextColor(_ color: UIColor) -> Builder 
```

### `withBackgroundColor(_:)`

``` swift
public func withBackgroundColor(_ color: UIColor) -> Builder 
```

### `withInvalidColor(_:)`

``` swift
public func withInvalidColor(_ color: UIColor) -> Builder 
```

### `withTextFieldTintColor(_:)`

``` swift
public func withTextFieldTintColor(_ color: UIColor) -> Builder 
```

### `withRadioButtonImage(on:off:)`

``` swift
public func withRadioButtonImage(on: UIImage, off: UIImage) -> Builder 
```

### `withCheckBoxOnImage(_:)`

``` swift
public func withCheckBoxOnImage(_ image: UIImage) -> Builder 
```

### `withBackButtonImage(_:)`

``` swift
public func withBackButtonImage(_ image: UIImage) -> Builder 
```

### `withActionButtonTitleFont(_:)`

``` swift
public func withActionButtonTitleFont(_ font: UIFont) -> Builder 
```

### `withActionButtonSubtitleFont(_:)`

``` swift
public func withActionButtonSubtitleFont(_ font: UIFont) -> Builder 
```

### `withTitleFont(_:)`

``` swift
public func withTitleFont(_ font: UIFont) -> Builder 
```

### `withActionButtonDetailsFont(_:)`

``` swift
public func withActionButtonDetailsFont(_ font: UIFont) -> Builder 
```

### `withBrandLogo(_:)`

``` swift
public func withBrandLogo(_ image: UIImage) -> Builder 
```

### `withPaymentCellBackground(_:)`

``` swift
public func withPaymentCellBackground(_ color: UIColor) -> Builder 
```

### `withPaymentCellTextColor(_:)`

``` swift
public func withPaymentCellTextColor(_ color: UIColor) -> Builder 
```

### `withPaymentCellDetailsTextColor(_:)`

``` swift
public func withPaymentCellDetailsTextColor(_ color: UIColor) -> Builder 
```

### `withPrimaryTextColor(_:)`

``` swift
public func withPrimaryTextColor(_ color: UIColor) -> Builder 
```

### `withPaymentCellMode(_:)`

``` swift
public func withPaymentCellMode(_ mode: PaymentCellIconsMode) -> Builder 
```

### `shouldHideLogo(_:)`

``` swift
public func shouldHideLogo(_ hideLogo: Bool) -> Builder 
```

### `shouldHideLogoOnResultAndLoadingScreen(_:)`

``` swift
public func shouldHideLogoOnResultAndLoadingScreen(_ hideLogo: Bool) -> Builder 
```

### `shouldShowCloseButton(_:)`

``` swift
public func shouldShowCloseButton(_ showCloseButton: Bool) -> Builder 
```

### `shouldCenterLogo(_:)`

``` swift
public func shouldCenterLogo(_ centerLogo: Bool) -> Builder 
```

### `shouldShowActionButtonLabels(_:)`

``` swift
public func shouldShowActionButtonLabels(_ showLabels: Bool) -> Builder 
```
