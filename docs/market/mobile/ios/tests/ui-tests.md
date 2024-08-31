# UI-Тесты

{% cut "Полезные ссылки на старую документацию" %}

[Wiki - Beru UI Tests](https://wiki.yandex-team.ru/users/khabiroff/beru-ui-tests/)

[Wiki - UI Test Guidelines](https://wiki.yandex-team.ru/users/andjash/ui-tests-guidelines/)

[Wiki - Flaky tests](https://wiki.yandex-team.ru/users/andjash/flakytests/)

{% endcut %}


## Общее

UI тесты для iOS приложения Маркета пишутся с помощью стандартного фреймворка [XCTest](https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/testing_with_xcode/chapters/09-ui_testing.html), сторонней библиотеки [AutoMate](https://github.com/PGSSoft/AutoMate), предоставляющей удобные расширения к стандартному XCTest API, Allure для сбора отчетов, и [GCDWebServer](https://github.com/swisspol/GCDWebServer) для подстановки в тест желаемых ответов сервера.

### Запуск 

Чтобы запустить тесты нужно выбрать схему `AllTests` и симулятор **iPhone 8** (на других симуляторах тесты могут падать и это нормально).

![setup](_assets/ui-tests/setup.png "Подготовка к запуску" =400x100)

Запустить все тесты - `Cmd+U`. Запустить конкретный тест можно нажав на иконку напротив него:

![launch_test](_assets/ui-tests/launch_test.png "Запуск теста" =400x200)

### Что проверяем?

XCTest — это фреймворк для функционального end-to-end тестирования. Тестами проверяется функциональность приложения: что действия пользователя выполняют нужную задачу (переход между страниц работат корректно, элементы кликабельны и видны на экране и т.д.). 

В функциональное тестирование не входит проверка дизайна (например "кнопка после нажатия покрасилась в белый цвет" или "высота надписи должна быть 3 строчки"), и XCTest **не предоставляет возможностей это проверить**.

### Лайфхаки

- При прогоне тестов локально и при написании нужно использовать только симулятор **iPhone 8**.
- При локальном прогоне нужно обязательно убедиться, что в симуляторе включена программная клавиатура (I/O > Keyboard > Toggle Software Keyboard).

- Если нужно пронать локально все тесты, то можно прогнать их _in parallel_ (см. скриншот).

{% cut "Как настроить параллельные тесты" %}

![in_parallel](_assets/ui-tests/in_parallel.png "In parallel option" =400x200)

{% endcut %}

- У каждого теста таймаут выполнения 2 минуты. Если локально тест проходит за такое время, то надо что-то менять, так как на CI будет дольше.
- При прогоне тестов на CI упавшие на первом прогоне тесты перепрогоняются еще раз, упавшие за второй прогон - еще раз, всего 3 раза.
- Прогон теста в CI можно отключить, указав его в `project.yml`. При отключении теста нужно обязательно завести тикет на его исправление и обратное включение. 
- Для просмотра результатов прогона UI тестов используется Allure репорт, который формируется при завершении джобы в CI - в нем можно найти скриншоты и шаги, на которых тест упал.
- В коде основного приложения есть механизм определения в тестовом окружении запущен проект или нет, для этого нужно использовать `TestLaunchEnvironmentKeys.insideUITests`.
- Для того, чтобы распечатать дерево элементов на экране нужно воспользоваться [debugDescription](https://developer.apple.com/documentation/xctest/xcuielement/1500909-debugdescription). При написании теста в нужный момент в коде можно распечатать иерархию с помощью `print(app.debugDescription)` или, поставив брейкпойнт, распечатать с помощью `po app.debugDescription` в debug area.
- Для выполнения нажатия по элементу с помощью `tap()` нужно сначала убедиться, что элемент удовлетворяет двум условиям `exists` и `hittable` (то есть `isVisible`). Для этого стоит воспользоваться `ybm_wait(forFulfillmentOf: { element.isVisible })` или `ybm_wait(forVisibilityOf: [element])` (если элемент виден в иерархии, то это не значит что он `isVisible`).

## Пишем первый тест

Данный процесс состоит из нескольких этапов:
1. Создаем файл для будущего теста в одной из папок `BlueMarketUITests/Tests/Cases` и объявляем класс теста.
2. Если необходимо (например, был добавлен новый UI элемент в иерархию вьюшек или же тест затрагивает элемент, которого нет в соответствующем [POM](#pom)) проставляем в коде `accessibilityIdentifier`. Более подробная инструкция как это сделать представлена в [разделе Accessibility Hierarchy](#accessibilityhierarchy).
3. Практически всегда при написании UI тестов приходится мокировать данные для получения нужных ответов API, подробнее о том как это делать и как вставлять моки в код теста описано в этом разделе про [моки](#mocks).
4. Иногда какая-то фича, которую необходимо покрыть тестом, находится под экспериментом или тогглом: что делать в таком случае описано [в разделе про тогглы](#toggle) и [эксперименты](#experiments). 
5. При написании самого теста проверки на существование/видимость элементов осуществляем с помощью ассертов XCTest. Если нужно подождать отображение элемента используем методы из `BlueMarketUITests/Extensions/XCTestCase+wait.swift`. Подробнее [тут](https://wiki.yandex-team.ru/users/andjash/UI-Tests-Guidelines/).

{% cut "Иерархия BlueMarketUITests" %}

![hierarchy](_assets/ui-tests/hierarchy.png "Иерархия" =200x400)

{% endcut %}

## Авторизация в тестах - AccountManager

Очень многие тест кейсы предполагают, что мы будем авторизованы в приложении при проверке сценариев корзины/чекаута и т.д. 

### Подход

Нам требуется управлять двумя вещами:
1. Авторизован ли пользователь
2. Есть ли у пользователя Я+

{% cut "Детали реализации в BlueMarketUITests" %}

Оба булевских поля можно прокинуть через переменные окружения (environment variables), таким образом на старте приложения подменив аккаунт тестовым `StubAccount`. Для этого мы должны указать следующие `env` переменные
1) Непосредственно указать в коде, что нам нужна авторизация через `StubAccount` (stubAuthorization = true), 
2) Запустить тесты (isRunningTests = true)

Эта информация проставляется в `func setUp()` базового класса тестов `LocalMockTestCase`, в базовом классе `TestCase`:
```swift
class TestCase: XCTestCase {

    /// Пользователь, который будет подменяться в АМ приложения
    var user: UserAuthState { .unauthorized }

    ...
}
```

```swift
class LocalMockTestCase: TestCase {

    override func setUp() {
        ...
        app.launchEnvironment[TestLaunchEnvironmentKeys.stubAuthorization] = String(user.isLoggedIn)
        app.launchEnvironment[TestLaunchEnvironmentKeys.hasYPlus] = String(user.isYandexPlus)
    }
}
```

{% endcut %}

При реализации нового теста в коде мок авторизации будет выглядеть следующим образом:

```swift
final class SomeTest: LocalMockTestCase {

    // Указываем что нам нужен Я+ аккаунт
    override var user: UserAuthState {
        .loginWithYandexPlus
    }

    func testSomething() {
        // Тут мы уже авторизованы с аккаунтом Я+
    }

    func testAnotherFeature() {
        // Тут мы также авторизованы с Я+
    }
}
```

Таким образом все опции авторизации, которые можно указать при переопределении переменной `user` в классе теста:
- .unauthorized (by default)
- .loginWithYandexPlus
- .loginNoSubscription

{% note info "Авторизация как часть теста" %}

Обычно авторизован пользователь или нет прописано в предусловии тест-кейса в `TestPalm`. Но есть случаи, которые требуют непосредственной авторизации в процессе тест кейса (прим. [testcase/bluemarketapps-3869](https://testpalm.yandex-team.ru/testcase/bluemarketapps-3869) - шаг 15 и 16), для таких случае используем метод `completeAuthFlow(with: .yandexPlusCredentials)`. Пример можно найти в `CheckoutFirstFlowTests`.

{% endnote %}

### StubAccountManager
В коде основного приложения BlueMarket реализация выглядит так:

{% cut "Класс StubAccountManager" %}

```swift
/// StubManager для мока AccountManager в приложении.
/// Используется вместо AccountManager в случаях запуска приложения для UI тестов.
final class StubAccountManager: YMTAccountManager {

    override var account: YALAccount? {
        get {
            guard
                let hasYandexPlus = testSettings?.hasYPlus
            else { return nil }

            return StubAccount(hasYandexPlus: hasYandexPlus)
        }
        set { super.account = newValue }
    }

}
```

{% endcut %}

При инициализации зависимостей в DI вместо `YMTAccountManager` подставляется (этим управляет `AccountManagerFactory`) `StubAccountManager`, данные для которого вытаскиваются из поля `testSetting` -- это настройки, которые передаются из тестов с помощью env переменных при `setUp()` теста. Эти настройки потом (в коде теста после `appAfterOnboardingAndPopups()`) подменить **нельзя**. 


## Мокирование данных { #mocks }

В iOS приложении Маркета моки подставляются путем создания сервера на локалхосте, который маппит path полученного HTTP запроса в название `.json` файла, который хранит желаемый ответ сервера с мок-данными. Такой подход позволяет протестировать весь network слой приложения. 

### Общие моки
Моки, по-умолчанию загружаемые в каждый тест (если он наследуется от `LocalMockTestCase`), предоставляют ответы сервера на запросы, которое приложение отправляет при старте, при загрузке морды, при открытии одной из SKU, при загрузке выдачи. Эти моки лежат в `BlueMarketUITests/Mocks/Local/MockSets/DefaultSet.bundle`. 

### Добавление моков в тест

Новый подход к мокированию данных описан [тут](#newmocks).

Файлы в мок-сервер в тестах, наследующихся от `LocalMockTestCase`, предоставляет класс `LocalMockStateManager` (поле `mockStateManager`). Чтобы собрать нужные для теста моки, можно использовать класс `LocalMockRecorder`, вот как это можно сделать:
1. Открыть в проекте `BlueMarketUITests/Mocks/Local/LocalMockRecorder.swift`
2. Раскомментить вызов `URLProtocol.registerClass(self)` в методе `enableIfNeeded()`
3. Собрать и запустить приложение в симуляторе
4. В приложении перейти в нужное состояние и наблюдать в логах пути, куда сохранены файлы ответов сервера
5. Открыть папку, указанную в логах, достать минимально необходимый набор файлов оттуда (сохраняться будут вообще все ответы на все запросы, отправленные приложением)
6. Создать бандл в `BlueMarketUITests/Tests/Cases/TestPalmEpic/Mocks/` и положить туда нужные файлы
7. В тесте в нужный момент вызвать метод `mockStateManager?.pushState(bundleName: "MyMocksBundle")` чтобы дополнить стандартный сет моков (если названия совпадают, файлы из указанного бандла заменят текущие)
8. **Закомментить `LocalMockRecorder` обратно!**

{% note warning "Возможные проблемы" %}

Так как мок-сервер подставляется в приложение путем замены `baseURL` запросов, а `baseURL` в некоторых случаях содержит `path` компоненты, в редких случаях названия записанных моков в обычном запуске и названия файлов, по которым будет искать мок-сервер, могут отличаться. Если приложение в тесте не использует нужный мок, можно поставить breakpoint в `LocalMockUtils.fileName(from:method:)` и посмотреть, какое название для запроса генерируется в тестовом и обычном запусках. 

{% endnote %}


### Изменение моков { #change-mocks }

_User-stories_ приложения могут зависеть от конкретной даты, пришедшей из моков (пример - появление `RateMe` попапа с момента получения заказа в течение 14 дней в секции "Мои заказы", [тест](https://testpalm.yandex-team.ru/testcase/bluemarketapps-967) `RateMePopupTest/testAfterDelivery()`). Из-за этого некоторые моки надо менять при каждом запуске теста для исправления данных (например, времени получения заказа в `..._user_orders_json.json`). Для этого был написан метод в `LocalMockStateManager`, цель которого - создать новый мок и подменить нужные данные. В параметрах метода проставляем по порядку имя бандла, откуда надо взять нужный мок, **новое** имя бандла с измененным моком, имя `.json` файла для изменения данных, тюпль с исправлением (само исправление, что на что менять). После этого надо замокать оба бандла, но новый с измененным моком запушить последним.

Также был создан вспомогательный класс `ChangeDateRegexBuilder`, который генерирует тюпль с исправлением даты (часто требуется для промокодов, акций, бонусов). Также можно воспользоваться протоколом `ReferralProgramMockHelper`.

{% cut "Пример использования `ChangeDateRegexBuilder`"%}

```swift
let endDate = ChangeDateRegexBuilder(key: "endDate", format: .yyyy_MM_dd, daysDiffSinceNow: 10)

"Изменяем дату окончания промокода на валидную".ybm_run { _ in
    mockStateManager?.changeMock(
        bundleName: oldBundleName,
        newBundleName: newBundleName,
        filename: "POST_api_v1_resolveSkuInfo,resolveProductOffers,resolveCms",
        changes: [
            endDate.buildChangeSet()
        ]
    )
}
```
{% endcut %}


```swift
mockStateManager?.changeMock(bundleName: oldBundleName,  
                             newBundleName: newBundleName,  
                             filename: filename,
                             change: (data, replacement))  

"Мокаем ручки".ybm_run { _ in
      mockStateManager?.pushState(bundleName: oldBundleName)
      mockStateManager?.pushState(bundleName: newBundleName)
}
```


Иногда появляется необходимость добавить правила, по которым можно определить какие моки в каких случаях заменяются. Структуру, позволяющую это сделать, можно найти в `MockMatchRule`. 
Пример использования можно найти в `DSBSCheckoutFlowTests`:
```swift
let blueSuppliersRule = MockMatchRule(
                id: "SUPPLIER_ID_BLUE",
                matchFunction:
                isPOSTRequest &&
                    isFAPIRequest &&
                    hasExactFAPIResolvers(["resolveSupplierInfoById"]) &&
                    hasStringInBody(#""isWhite":false"#),
                mockName: "supplierInfoById_blue"
            )
            let whiteSuppliersRule = MockMatchRule(
                id: "SUPPLIER_ID_WHITE",
                matchFunction:
                isPOSTRequest &&
                    isFAPIRequest &&
                    hasExactFAPIResolvers(["resolveSupplierInfoById"]) &&
                    hasStringInBody(#""isWhite":true"#),
                mockName: "supplierInfoById_white"
            )
            mockServer?.addRule(blueSuppliersRule)
            mockServer?.addRule(whiteSuppliersRule)
```

### Мокирование экспериментов { #experiments }

В тестовом запуске приложение будет ждать выполнения запроса на получение экспериментов и применять их моментально, если в `launchEnvironment` передано `true` по ключу `TestLaunchEnvironmentKeys.waitForExperiments`, только после этого показывая какой-либо UI. 

{% cut "Пример - legacy"%}

Для подстановки нужных экспериментов в приложение, нужно собрать .json файл с экспериментами точно так же, как описано в предыдущем разделе, и запушить бандл с экспериментами в тесте **до запуска приложения**.

```swift
app.launchEnvironment[TestLaunchEnvironmentKeys.waitForExperiments] = String(true)
        
"Мокаем startup для получения эксперимента FAPI_SKU_METHOD_IOS_TEST".ybm_run { _ in
    mockStateManager?.pushState(bundleName: "Experiments_SKU_FAPI")
}

app.launch()
```

{% endcut %}

На новом механизме мокирования есть замоканная ручка `ResolveBlueStartup`, с помощью которой можно мокировать эксперименты внутри теста следующим образом:

```
var defaultState = DefaultState()

app.launchEnvironment[TestLaunchEnvironmentKeys.waitForExperiments] = String(true)

"Мокаем startup для получения эксперимента express_fix_control (компактное саммари)".ybm_run { _ in
    defaultState.setExperiments(experiments: [.expressFixControl])
    stateManager?.setState(newState: defaultState)
}
```

P.s: возможен кейс написания тестов-экспериментов для уже существующих тестов. Для этого в старых тестах надо также указать строчку 
```swift
app.launchEnvironment[TestLaunchEnvironmentKeys.waitForExperiments] = String(true)
```

### Мокирование тогглов { #toggle }

В файле `BlueMarketUITests\Generated\FeatureNames.generated.swift` перечислены объявленные в приложении тогглы. 
Включить тоггл в тесте можно следующим образом:
```swift
enable(FeatureNames.cashback)
```
Выключить так:
```swift
disable(FeatureNames.cashback)
```
Для включения нескольких тогглов:
```swift
enable(FeatureNames.cashback,
       FeatureNames.plusBenefits)
```
Также для тогглов можно пробрасывать дополнительные данные (примеры можно найти в `PlusOnboardingTestCase` и `HelpIsNearCheckoutTestCase`):
```swift
app.launchEnvironment[TestLaunchEnvironmentKeys.enabledTogglesInfo] = ...
```

{% note info "Примеры в коде" %}

- Примеры включения нескольких тогглов можно найти в тестах `PlusOnboardingTestCase`,
- Пример проброса дополнительных данных можно найти в тестах `PlusOnboardingTestCase`, `HelpIsNearCheckoutTestCase`,
- При тестировании иногда нужно будет включать одни и выключать другие тогглы, пример можно найти в `CashbackInYandexPlusTestCase`.

{% endnote %}

## Работа с Accessibility Hierarchy { #accessibilityhierarchy }

⚠️ **Очень полезно**: `print(app.debugDescription)` или `print(XCUIApplication().debugDescription)` - так можно распечатать иерархию элементов в любой момент времени.

XCUITest ориентируется в приложении с помощью иерархии доступности. Эта иерархия сильно проще иерархии View. Чтобы получить элемент из иерархии сначала делается запрос (`XCUIElementQuery`), который выполняется только после совершения каких-то действий с элементом. Чтобы переиспользовать такие запросы и не городить их в тестах, используется паттерн PageObject. Подробнее о `PageObject` описано в [соответствующем разделе](#pom).
Элементы из Accessibility Hierarchy достаются чаще всего по `accessibilityIdentifier`'ам. Константы для этих значений хранятся в отдельных классах, доступных как в тестовом, так и в основном таргете. Файлы с константами для идентификаторов доступности хранятся в папке `BlueMarket/Classes/Accessibility`.

Расстановка `accessibilityIdentifier`-ов не самая тривиальная задача, так как часто в иерархии элементов нужный элемент просто сткрыт. 
Для разметки элементов в коде стоит руководствоваться следующими принципами:
1. Если нужно проставить элементы у вьюшек, то как правило создается `extension` с соответвующим `MARK: - Accessibility`, в котором находится метод `setUpAccessibility()` (см. примеры в коде);
2. `.isAccessibilityElement = true` - чтобы добраться до элементов, скрытых глубоко в иерархии;
3. Также можно напрямую указать все элементы в контейнере, которым в последующем надо будет назначить `accessibilityIdentifier`. Это можно сделать с помощью [accessibilityElements](https://developer.apple.com/documentation/objectivec/nsobject/1615147-accessibilityelements);

### Page Object Model {#pom}

При написании UI тестов используется шалон `Page Object`, с помощью него можно создать в коде представление иерархии элементов страницы приложения. Классы с POM страниц лежат в папке `/BlueMarketUITests/Tests/POM`, пример можно посмотреть в `FeedPage` - POM экрана выдачи. Все `POM`-ы страниц наследуются от общего класса `PageObject` и представляют собой набор полей и методов для получения элементов. Внутри `POM`-ов также можно объявлять nested классы, которые относятся непосредственно к соответсвующему `POM`-у (и только к нему). В `PageObject` можно класть запросы до нужных элементов, частые действия с ними (переход куда-то и так далее), но не тестирование (ассертов быть там **не должно** — это часть логики самих тестов, но их можно встретить и желательно выпиливать, вставляя ассерты в сами тесты). Подробнее о часто используемых `POM`-ах (`CollectionManagerPage`) и работе с ними есть описание в разделах ниже.

### Работа с коллекциями

В отличие от `UITableView`, `UICollectionView` не позволяет найти ячейки, которые еще не появились на экране. Если в таблице, в которой лежит 10 ячеек, из которых видны 3, сделать запрос `table.cells.matching(identifier: "myCell").allElementsBoundByIndex`, то вернется массив с 10 элементами. Коллекция в тех же условиях вернет 3-4 элемента, которые по мере скролла будут ссылаться на разные ячейки. Но в больших таблицах `UITableView` предпочтительно не использовать `allElementsBoundByIndex`, а пользоваться API `CollectionManagerPage`.

{% note alert "Bad practice" %}

В данный момент в большом количестве тестов на выдачу используется следующая строчка:
```swift
let snippetPage = feed.collectionView.cellPage(at: IndexPath(item: 0, section: 5))
```
Написание новых фич (добавление новых ячеек из FormKit) может сильно [поломать UI тесты](https://st.yandex-team.ru/BLUEMARKETAPPS-24817), так как количество ячеек изменится и нужно будет везде менять индекс. 
В данном случае best practice будет модифицировать POM страницы выдачи и назначить осмысленный accessibilityIdentifier, не доставая ячейки по индексу. 

{% endnote %}

#### CollectionManagerPage 

Специально для работы с коллекциями в рамках иерархии доступности был написан класс `CollectionViewCellsAccessibility` для назначения идентификаторов доступности ячейкам коллекций (например в `cellForItemAt:`) и протокол `CollectionManagerPage` для автоматической имплементации запросов к ячейкам по индексам. 
- `cellElement(at indexPath: IndexPath)` - метод для поиска элементов-ячеек в коллекции по индексам, используется для итерации по ячейкам коллекции когда известно их положение в иерархии и надо проверить массив ячеек. Этот метод будет отдавать нужные ячейки при условии, что они видны на экране (в случае с CollectionViewPage надо быть осторожным) , поэтому перед получением ячейки до нее нужно сначала доскроллить (с помощью `ybm_swipe(toFullyReveal:)`).
- `cellUniqueElement(withIdentifier identifier: String)` - метод для получения ячейки коллекции по уникальному идентификатору. Позволяет не привязываться к положению ячейки в коллекции. 

> Оба метода могут работать вместе. Идентификатор ячейки выглядит как `SKUCardAccessibility.galleryCell-0-1-SpecialId-`. Постфикс `-SpecialId-` позволяет не привязываться к положению ячейки в коллекции, а вытаскивать ее из иерархии по значению.

#### Коллекции с одинаковыми элементами

Как расширение для `CollectionManagerPage` есть `UniformCollectionManagerPage`, где с помощью `associatedtype CellPage: PageObject` можно указать конкретный тип ячейки коллекции. Таким образом оба метода для доступа к ячейкам будут возвращать не просто `XCUIElement`, а конкретный объект класса `CellPage`.


#### Работа с FormKit
Много экранов в приложении (экраны шагов чекаута, часть КМ, ...) сверстано с помощью механизма `FormKit` с использованием `CellController`, `SectionControlller`. Для расставления `accessibilityIdentifier` на ячейки/секции из `FormKit` используется :

```swift
/// Поле, определяющее accessibilityIdentifier на элементы ячеек
/// в секции с использованием indexPath
public var classForAccessibility: AnyClass?
``` 

Примеры работы c ячейками форм кита можно посмотреть в 
- `FeedBasicTest/testBasicFunctionality()` - достаем ячейку по _indexPath_; 
- `SKUCardModelPreorderTest/testAuthorized()` - кнопка "Продолжить" при переходе с одного шага чекаута на другой, достаем по _uniqueIdentifier_;

### Поиск элементов в иерархии

Accessibility Hierarchy, которую XCTest использует для поиска элементов, — это сильно упрощенная View Hierarchy. Эта иерархия состоит из контейнеров (`UIAccessibilityContainer`) и элементов (`UIAccessibilityElement`). Контейнеры будут видны в иерархии при тестировании, но не будут доступны при использовании приложения через VoiceOver.
Для нахождения элементов интерфейса в тестах, используется `XCUIElementQuery` — набор критериев, по которым мы ищем элемент(ы). Элементы из запросы можно доставать по индексу или по `accessibilityIdentifier`.
При работе с иерархией доступности можно порассуждать, как бы взаимодействовал с приложением человек, использующий VoiceOver. Один Accessibility Element может включать в себя несколько UIView. Например если кнопка состоит из иконки, надписи и фона, то это не значит, что нужно делать 3 элемента доступности — для функционального тестирования и для слабовидящих пользователей будет достаточно понять, что это одна единая кнопка со значением `acessibilityLabel` равным тексту в надписи, и не нужно делать ее контейнером.
Примеры контейнеров и элементов:
400x0:https://wiki.yandex-team.ru/users/khabiroff/beru-ui-tests/.files/screenshot2019-07-03at15.45.57.png
В этом случае можно было соединить в один элемент иконку, заголовок и подзаголовок. В таком случае мы сможем проверить в тесте текст заголовков и подзаголовков через `accessibilityLabel` элемента. Вью, в котором лежит этот элемент, является контейнером, и не выделяется через VoiceOver (но доступен в тестах). Таким образом можно получить достаточные для функционального тестирования возможности, сделав удобный интерфейс для пользователей с ограниченными возможностями.

**Реализация контейнеров:**
```swift
class TestContainerView {
    override func awakeFromNib() {
        super.awakeFromNib()
        isAccessibilityElement = false
        accessibilityElements = [
            ...
            // перечисляем сабвьюхи, которые должны быть видны в иерархии (элементы) 
            //или другие контейнеры
        ]
        accessibilityIdentifier = TestAccessibility.container // идентификатор, чтобы делать query
        for element in accessibilityElements {
            
        }
    }
}
```

**Реализация элементов:**
```swift
class TestElementView {
    override func awakeFromNib() {
        // дефолтное значение во многих случаях
        // если задаете руками — после этого все сабвьюхи перестанут быть видны
        // в accessibility hierarchy
        isAccessibilityElement = true
        accessibilityIdentifier
    }
}
```

Для дебага удобно использовать Accessibility Inspector, View debugger и команды lldb, поставив брейкпоинт прямо в тесте и позадавав разные query на искомый элемент.

### Скролл
Скроллинг в тестах нужен в случаях, когда нужно проверить видимость каких-либо элементов. Чтобы проверить правильность текстов в них, скроллингом можно пренебречь — тексты можно получить даже когда элемент не виден на экране устройства, **при условии, что этот элемент не находится в ячейке `UICollectionView`**. 
Так как в `UICollectionView` по запросу `.cells` отдаются не все, а только видимые в пределах одного фрейма коллекции ячейки, для проверки элементов, которые изначально не лезут в коллекцию без скролла, придется присвоить им `accessibiltyIdentifier`, хранящий в себе `IndexPath` (для этого можно посмотреть как работает и используется класс `CollectionViewCellsAccessibility`), и физически до него скроллить.
Скроллить можно с помощью нескольких методов. Самый "нативный" и быстрый — тапнуть элемент. Если элемент не виден на экране хотя бы частично, `XCUIElement.tap()` проскроллит к этому элементу, если возможно. Этот способ самый надежный, но не все элементы можно тапнуть, а в некоторых случаях могут быть последствия (например нужно проскроллить к кнопке, а нажатие на нее приведет к переходу на другой экран). В таких случаях есть методы `XCUIElement.swipe(...)` (из AutoMate), предлагающие несколько вариантов скроллинга, и `ybm_swipe(toFullyReveal:)`, который медленно, но аккуратно скроллит к нужному элементу. Способы использования этих методов и их документацию можно почитать в проекте.

### Тап

При реализации функции `func tap() -> ...` в `EntryPoint` или где бы то ни было еще возможен кейс, когда ячейка не тапается в `element.tap()`. Как главная причина - офсет. Для этого случая подойдет `element.coordinate(withNormalizedOffset: CGVector(dx: 0.1, dy: 0.1)).tap()` на замену обычному `element.tap()`

## Оценка задач по написанию UI тестов
При оценке задач по написанию тестов следует учитывать сложность View иерархии тестируемых элементов, имеются ли PageObject'ы для тестируемых элементов (здесь нужно учитывать и путь к этим элементам, так как открыть тестируемую область приложения нужно будет тоже с их помощью), и требуются ли дополнительные моки. Практика показывает что в среднем 1 UI тест занимает 2-3 SP, но в редких случаях может затянуться и до 5.

## Возможные проблемы
### Не подгружаются мокированные данные, ошибка "Что-то пошло не так"

Если запущен отладочный прокси-сервер (Charles, Proxyman и т.п.), он может препятствовать работе GCDWebServer, который раздаёт мокированные данные.
Решение — временно пустить системный трафик в обход отладочного прокси:
* Charles: Proxy - macOS Proxy
* Proxyman: Tools - Proxy Settings - Override macOS Proxy
* Универсальное решение: System Preferences - Network - выбрать текущую сеть - Advanced - Proxies - снять чекбоксы с HTTP и HTTPS.


# UI Tests Guidelines

В данном разделе перечислены частые проблемы и способы их решения.

{% cut "Проверка равенства строк" %}

- Как **не нужно** делать

Сравнивать со строками, сгенерированными из ресурсов (L10n).

- Как делать

Сравнивать с константами.

{% note warning "" %}

Все из-за выкачки строковых ресурсов из Tanker-а. Если там менятся - нужно об этом знать в приложении и фейлить тест.

{% endnote %}

{% endcut %}



{% cut "Неадекватный отчет в Allure" %}

- Как **не нужно** делать

Не использовать ybm_wait(forFulfillmentOf: для чего либо кроме ожидания показа.

- Как делать

Использовать XCTAssertEqual.

{% note warning "" %}

Из-за проверки всего условия разом и одного generic ассерта непонятно какое именно из сравнений сфейлилось. Также, по умолчанию для метода установлен таймаут в 10 секунд, которого может не хватить на множество проверок. В результате можем получить флаки тест.

{% endnote %}

{% endcut %}


{% cut "Тест функционала с датой" %}

- Как **не нужно** делать

Тестировать, завязываясь на текущую дату.

- Как делать

Перезаписывать дату в моке при каждом запуске теста. Смотри раздел [по изменению моков](#change-mocks).

{% note warning "Пример" %}

`RateMePopupTest.testAfterDelivery`

{% endnote %}

{% endcut %}


{% cut "Нет в иерархии accessibility UICollectionView, которая лежит в UITableViewCell" %}

- Как делать

Обернуть UICollectionView в UIView.

{% note warning "" %}

[Пишут](https://stackoverflow.com/a/38798448), что баг эпла.

{% endnote %}

{% endcut %}



{% cut "Проверка кол-ва ячеек в UICollectionView или проверка i-й ячейки определенного типа" %}

- Как **не нужно** делать

Не нужно делать поиск по индексу, если это не коллекция одинаковых ячеек (выдача, заказы).

- Как делать

Делать подскрол к 1-й ячейке. Начиная с нее делать поиск нужного accessibilityIdentifier-а, начиная с уже пройденных.

{% note warning "" %}

Нормальные итерации невозможны т.к. ячейки перестают существовать при их проскролле => проверка на count будет падать. Использовать при этом cellUniqueElement(withIdentifier:after:). Например - `SKUCardModelOpinionsTest.testThreeOpinions`.

{% endnote %}

{% endcut %}


{% cut "Флакает тест" %}

- Как делать

Не писать тесты в синхронном стиле. Всегда учитывать что ui элемент может появится не сразу, а через какое-то время. Для проверки можно использовать Slow Animations на симуляторе.

{% endcut %}

{% cut "Флакает тест - проблемы с ybm_wait" %}

- Как **не нужно** делать

Ненужно делать `ybm_wait(forFulfillmentOf...` с очень длинным условием. Т.к. это условие очень долго выполняется, на слабых машинках тест может иногда падать

```swift
ybm_wait(forFulfillmentOf: {
    popup.element.isVisible
    && popup.goToCartButton.element.isVisible
    && popup.image.exists
    && popup.title.label == popupData.title
    && popup.name.label == popupData.sku.title
    && popup.currentPrice.label == "611 ₽  "
    && popup.continueShoppingButton.label == popupData.continueShopping
    && popup.goToCartButton.element.label == popupData.goToCartButton
    && popup.freeDelivery.title.label == popupData.recomendations.delivery
})
```

- Как делать

Делаем один wait на появляение нужного экрана/контенейра/ячейки. После этого делаем assert для проверки данных.

```swift
ybm_wait(forFulfillmentOf: { popup.element.isVisible })
XCTAssert(popup.goToCartButton.element.isVisible)
XCTAssert(popup.image.exists)
XCTAssertEqual(popup.title.label, popupData.title)
XCTAssertEqual(popup.name.label, popupData.sku.title)
XCTAssertEqual(popup.currentPrice.label, "611 ₽  ")
XCTAssertEqual(popup.continueShoppingButton.label, popupData.continueShopping)
XCTAssertEqual(popup.goToCartButton.element.label, popupData.goToCartButton)
XCTAssertEqual(popup.freeDelivery.title.label, popupData.recomendations.delivery)
```


{% note warning "" %}

Это еще и плохо тем, что если какое-то из условий упадет, то все выражение будет false - это не дает никакой информации для отчета.

{% endnote %}

{% endcut %}


{% cut "Тест не проходит если его запустить в одиночку" %}

- Как делать

Всегда быть готовым к тому, что тест будет запускаться один. Подготавливать нужные экспы/тоглы, быть готовым к системным алертам и тд.

{% endcut %}


{% cut "Проблемы с паспортом" %}

Ходим в сеть, при изменении оси/паспорта приходится переписывать тесты.

- Как делать

Считаю что проходить полный флоу в паспорте нет смысла и в тестах его достаточно открыть и закрыть.

{% endcut %}


{% cut "Не находит элемент, хотя он присутствует в иерархии" %}

- Как делать

Пробуем найти элемент от XCUIApplication.

{% endcut %}




{% cut "Тесты на метрику" %}

- Как **не нужно** делать

```swift
let openEvent = self.analyticsEventsStorage?.getEvents("PRODUCT_PAGE_OPEN").first { event in
    guard let skuId = event.params["skuId"] as? String else { return false }

    return skuId == "100324823773"
}

XCTAssertNotNil(openEvent)
```

- Как делать

```swift
let openEvents = self.analyticsEventsStorage?.getEvents("PRODUCT_PAGE_OPEN").filter { event in
    guard let skuId = event.params["skuId"] as? String else { return false }

    return skuId == "100324823773"
}

XCTAssertEqual(openEvents?.count, 1)
```

{% note warning "" %}

Важно проверять не только факт отправки той или иной метрики, но и количество этой самой метрики

{% endnote %}

{% endcut %}



{% cut "Изменение фрейма у элемента с isHidden=true приводит в UI тестах к isVisible == true" %}

- Как делать

Поставить alpha в 0.

{% endcut %}

## Новый подход к мокированию данных { #newmocks }

Тикет проекта: https://st.yandex-team.ru/MARKETPROJECT-5629

В данный момент при написании теста нужно записывать `JSON` моки (ответы сервера) и складывать их в `.bundle`. Подробнее о существующем подходе можно прочитать [тут](#mocks).

### Анализ существующих решений

Существуют несколько способов по мокированию данных для UI-тестов приложения:
1. Уже упомянутая группировка записанных JSON ответов от сервера
2. Сериализация DTO моделей в качестве ответа от сервера
3. Тестирование с помощью [кадавра](https://wiki.yandex-team.ru/users/b-vladi/kadavrik/) - сервис, который генерирует моки в ответ на запросы. В таблице сравнения не рассматриваю, так как это больше про интеграционное тестирование.

Критерий сравнения | JSON моки | DTO моки
:--- | :--- | :--- |
Изменение моков | Тяжело, так как надо перебирать все json файлы | Легко, так как нужно изменить саму структуру (остальное подскажет компилятор) 
Ревью | Тяжело ревьюить 10k+ строк (и практически никто это не делает) | Есть константы, у которых меняем отдельные поля, легко ревьюить  |
Прозрачность | Большие JSON-ы, непонятно что используется внутри теста | Состояние видно прямо в коде теста |
Парсинг | Тестируется  | Тестируется, так как DTO сериализуются и десериализуются как реальные ответы, полученные от сервера
Легкость написания | Достаточно легко записать, вставив json-ы ответов от сервера | Можно переиспользовать константы (легко), иногда придется дополнять модели данными (терпимо) или делать новые (+- терпимо)

Было принято решение изменить подход к использованию моков в UI-тестах, так как:
1. При активном перезде с CAPI на FAPI тяжело чинить тесты.
2. При изменении респонса с сервера надо перебирать все `.json` моки и в каждом менять отдельные поля.
3. Записанные моки никто не ревьюит, поэтому это часто 10k+ строк, из которых фактически используется в тесте только 1k. Невозможность ревьюить моки потом плохо сказывается на пунктах 1 и 2.

Таким образом были выбраны `DTO` моки и следующая стратегия по их внедрению:

Все моки в bundle-ах (в json-ах) мы переводим в `DTO` объекты и когда нужно отдать ответ серверу `GCDWebServer` - сериализуем нужные нам модельки с данными и отдаем это в оболочке `GCDWebServerDataResponse`. 

### Мокирование запросов { #newmockshowto }

Если нужная ручка не замокана на новом механизме, то нужно:

1. Создать в `Mocks/Templates/Resolvers` в соответствующей папке файл, в котором будет находиться структура для CAPI и/или FAPI, из которой будет собираться и сериализовываться нужный нам ответ. 

{% note info "Примеры" %}

Пример запроса для FAPI - `ResolveUserOrders`

Пример запроса для CAPI - `ResolveBlueStartup`

Пример запроса для обоих бекендов - `ResolveSearch`

{% endnote %}

2. Создать необходимые структуры ответа от сервера, поместить их в `extension`. Для того, чтобы легко сделать структуру из JSON можно воспользоваться [quicktype](https://app.quicktype.io). JSON достаем из уже существующих JSON моков к тесту или из charles.
3. Изменить структуру одного из `Mocks/Templates/States` (подробнее в разделе о [состоянии приложения](#state)): добавить список `handlers` замоканный хэндлер, сделать публичный метод, чтобы менять его состояние.
4. В коде теста создать состояние и изменить соответсвующим образом, засетить в `stateManager?.setState(newState: feedState)`.

{% note info "Примеры тестов на новом механизме" %}

`MissingOrderItemsTests`, `testCashbackInProductSnippet()`, `testEmptyCartHeader()`

{% endnote %}

### LocalStateManager

Введем понятие `State` - набор состояний для основных экранов приложения, которые задаются при старте теста. Подробнее о состоянии написано [в этом разделе](#state).

Данное понятие уже активно используется для существующего механизма моков, стейт == бандл(-ы), которые на первых шагах теста мы отправляем `LocalMockStateManager`.

У запущенного теста есть дефолтный набор ([DefaultState](#defaultstate)) стейтов (OrdersState, AuthState, CartState, ...) - его можно подменять / изменять. 

Подмена в тесте будет происходить также как сейчас:
```
"Мокаем состояние".ybm_run { _ in
    // mockStateManager?.pushState(bundleName: bundleName) <--- БЫЛО
    stateManager?.setState(newState: ordersState) <--- СТАЛО
}
```

Пример: для создания стейта заказов, нужно выше в коде объявить какое-то состояние для резолверов (в данном случае набор заказов):

```
// Маппер для резолверов
let mapper = OrderHandlerMapper(orders: [
    SimpleOrder(
        status: .delivery
    )
])

// Создаем состояние
var ordersState = OrdersState()
// Меняем резолверы: resolveUserOrders, resolveUserOrdersById
ordersState.setOrdersResolvers(mapper: mapper, for: [.all, .byId])

"Мокаем состояние".ybm_run{
    stateManager?.setState(newState: ordersState)
}
```

### Шаги перевода существующих тестов с json на DTO

**Case 0**: Мокирование новых запросов описано в [этом разделе](#newmockshowto).

**Case 1**: Существующая ручка уже замокана (по новому), ее надо перевести с CAPI на FAPI

Одна из проблем, которую решают моки `DTO` - (почти) безболезненный переезд с `CAPI` на `FAPI`.
До сих пор есть ручки в приложении (прим. `user_orders_options`), которые ходят в CAPI и используются практически в каждом тесте на Корзину, Чекаут, т.д.

1. В существующем резолвере меняем реализуемый протокол на `FAPIHandler`, объявляем нужные методы/переменные
2. Меняем капишную структуру ответа на фапишную: из charles берем нужный нам json и смотрим какие поля в структуре нужно добавить/изменить.

{% note info "Примеры переезда с CAPI на FAPI" %}

[Пример ПР-а с переездом](https://github.yandex-team.ru/market-mobile/blue-market-ios/pull/10007) с CAPI `categories/0/popular/products` на FAPI `resolvePopularProducts`.

{% endnote %}

**Case 2**: Нужны тесты и на CAPI запрос, и на аналогичный FAPI запрос c тогглом.

Для нужного резолвера используем протокол `UniversalHandler`: он объединяет в себе `CAPI-` и `FAPIHandler`, а также задает тип ручки, которую нужно будет дергать в определенном тесте.

```
/// Universal Handler for CAPI & FAPI compatible handlers
protocol UniversalHandler: FAPIHandler, CAPIHandler {

    var requestType: Handler { get }
}
```

В этом протоколе также переопределении метод из `EndpointHandler`: `func handle(request: GCDWebServerRequest?) -> GCDWebServerResponse?` -- в зависимости от типа handler-а (.capi или .fapi) отдаем нужное тело request-а.

{% note info "Примеры" %}

Пример в коде можно найти для запроса на поиск `ResolveSearch` в `Mocks/Templates/Resolvers/Feed/search.swift`. 

Примеры в тестах можно найти в `FeedGiftsTest/testGiftVisible` (CAPI), `FeedExpressTest` (FAPI), `FeedBasicAuthTest.testCashback()` (FAPI).

{% endnote %}

### Состояние приложения { #state }

По аналогии с `LocalMockStateManager` была создана структура по управлению состояниями тестового приложения. В новом `LocalStateManager` состояния берутся не из бандлов, как раньше, а представляются объектами, содержащими список хэндлеров и публичных методов, которые могут менять ответы хэндлеров.

Все состояния приложения (выдача, sku, чекаут...) хранятся в `Mocks/Local/States`.

{% cut "Пример состояния выдачи `FeedState`" %}

```
struct FeedState: LocalState {

    /// Хэндлеры выдачи: фильтры, поиск, редирект
    var handlers: [EndpointHandler?] {
        [
            redirectHandler,
            searchHandler,
            searchFilterHandler
        ]
    }

    // MARK: - Public

    /// Устанавливаем стейт для редиректа выдачи
    /// - Parameter mapper: данные для выдачи (товары, сортировка)
    mutating func setRedirectState(mapper: Redirect) {
        redirectHandler = .init(capiRedirect: mapper)
    }
}
```

{% endcut %}

### DeafultState - дефолтное состояние приложения { #defaultstate }

При использовании подхода мокирования с помощью JSON мы использовали `DefaultSet.bundle`, в котором находилось **более 70** JSON-ов с такими базовыми запросами, как `resolvePopularProducts`(для скроллбоксов), `resolveSearch` (для поиска) и т.д.

При переходе на новый способ мокирования нужно было избавляться от `DefaultSet.bundle` в пользу стуктуры `DefaultState`, которая аналогично остальным состояниям в `Mocks/Local/States` инициализирует группу хэндлеров.
 
Детальный список всех запросов, которые мокируются в дефоолтном состоянии можно найти в `DefaultState`, если коротко, то:

1. _Морда_: баннеры, виджеты на морде.
2. _Заказы_: список всех и недавних заказов, опции заказа.
3. _Корзина_: наполнение корзины, бонусы и трешхолд.
4. _SKU_: скроллбокс, Q&A, отзывы.
5. _Выдача_: поиск, редирект, фильтры.
6. _Любимые категории_.


{% note warning "ATTENTION" %}

В `DefaultSet.bundle` больше не добавляем JSON-ы !!!

Если нужно дополнить дефолтное состояние, модифицируем `DefaultState`. Алгоритм как добавить новые моки описан [тут](#newmockshowto)

{% endnote %}


