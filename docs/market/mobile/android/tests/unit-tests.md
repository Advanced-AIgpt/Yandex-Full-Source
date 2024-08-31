# Страница про то, как писать unit-тесты в Android приложении Маркета

## Зачем писать

1. Чтобы проверить свой код
2. Чтобы защитить свой код от того, что кто-то внесёт правки и сломает его
3. Ради чистоты кода. Unit-тесты сложно писать на плохом коде. Если код написан хорошо, то тесты писать легко и приятно
4. Для документации. Часто тесты описывают необходимое поведение. Из этого становится ясно, как код должен работать


## Когда писать тесты

1. Обязательно покрываем сложную логику-алгоритмы
2. Покрывать общие части (Сетевой слой, навигация и т.п.)
3. Покрывать доменную-бизнес логику. Особенно сложную
4. Пока не покрываем view unit-тестами

## Как создать/где писать

1. Переводим курсор на название класса, который хотим протестировать
2. Нажимаем Alt + Enter
3. Выбираем Create test
4. Должно быть, как на [фото](https://jing.yandex-team.ru/files/apopsuenko/2021-04-16T08:04:06Z.5c154ee.png)
5. Нажимаем OK
6. Выбираем директорию .../market/src/**test**/ru/yandex...

## Как запускать

1. Определённый тест можно запустить просто нажав стрелочку
Выберите класс теста. В классе можно запустить всё или конкретный сценарий
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-04-16T08:23:39Z.58cc8e0.png)

2. Все тесты можно запустить через gradle-таску test(Qa|Base)Debug. Пример `./gradlew testBaseDebug`

3. Можно запустить несколько тестов (выбрать папку или несколько тестов), еси нажать на правую кнопку на папке и выбрать `Run tests`
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-04-16T08:25:14Z.46eee09.png)

4. Запустить тесты с замером покрытия можно через команду `./gradlew testBaseReleaseUnitTestCoverage -PcodeCoverage=true`. 
Отчёт будет лежать в `market/build/reports/jacoco/testBaseReleaseUnitTestCoverage/html/index.html`

## Как писать

Неплохой доклад от Яндексоида
@[youtube](https://youtu.be/MS7GN2Lgdas)

### Термины

- **_тест_** - можно понимать как класс с методами, так и метод внутри класса, которые тестируют
- **_зависимость_** - объект, который требуется тестируемому классу на вход
- **_мок_** - экземпляр класса, который не выполняет реальные методы, которые на нём вызываются. Нужен для подмены реализации и отслеживания взаимодействий с данным экземпляром
- **_сценарий_** - метод в классе теста, который помечен аннотацией `@Test`

### Структура теста

1. Имя класса теста: `[имя класса]Test`
2. Подготовка зависимостей. Здесь мокаются зависимости, которые нужны на вход тестируемому классу.
3. Подготовка правил. `Rule` позволяет оборачивать код, который будет выполняться в тесте и делать что-то перед и после сценария. А вынос этого кода в класс `Rule` позволяет переиспользовать код в правиле
4. Блок `@BeforeClass`. Аннотация `@BeforeClass` позволяет поместить в метод код, который будет выполняться 1 раз перед запуском всех сценариев
5. Блок `@Before`. Аннотация `@Before` позволяет поместить в метод код, который будет выполняться перед каждым сценарием
6. Блок `@After`. Аннотация `@After` позволяет поместить в метод код, который будет выполняться после каждого сценария
7. Блок `@AfterClass`. Аннотация `@AfterClass` позволяет поместить в метод код, который будет выполняться 1 раз после запуска всех сценариев
8. Сценарии

Имя сценария: описание сценария в виде "То-то происходит, когда такая-то ситуация". Пример - "Return empty string for offers count when there is only one offer"

В методе теста:
- начало. Описывается окружение, что возвращают зависимости в данном сценарии
- вызов тестируемых методов
- проверка возвращённого значения, вызванных методов зависимостей и т.д.

Пример
```kotlin
// 1. Имя класса
class ProductOfferFormatterTest {

    // 2. Подготовка зависимостей
    private val resourcesDataStore = mock<ResourcesDataStore>()
    private val reasonsFormatter = mock<RecommendedReasonsFormatter>()
    private val pricesFormatter = mock<PricesFormatter>()
    private val moneyFormatter = mock<MoneyFormatter>()
    private val experimentManager = mock<ExperimentManager> {
        on { getExperiment(SecretSaleSkuSplit::class.java) } doReturn object : SecretSaleSkuSplit {
            override val isSkuLabelVisible: Boolean = true
        }
    }
    private val discountFormatter = mock<DiscountFormatter>()
    private val offerPromoFormatter = mock<OfferPromoFormatter>()
    private val formatter = ProductOfferFormatter(
        resourcesDataStore,
        reasonsFormatter,
        pricesFormatter,
        discountFormatter,
        moneyFormatter,
        experimentManager,
        offerPromoFormatter
    )

    // 3. Подготовка правил
    @get:Rule
    var thrown: ExpectedException = ExpectedException.none()

    // 4. Блок BeforeClass
    @BeforeClass
    fun setUp() {
        // doSomething before all test cases
    }

    // 5. Блок Before
    @Before
    fun setUp() {
        // doSomething before every test case
    }

    // 6. Блок After
    @After
    fun setUp() {
        // doSomething after every test case
    }

    // 7. Блок AfterClass
    @AfterClass
    fun setUp() {
        // doSomething after all test cases
    }

    @Test
    // 8. сценарий
    // имя сценария
    fun `Return empty string for offers count when there is only one offer`() {
        // начало
        whenever(discountFormatter.format(any<Offer>())).thenReturn(DiscountVo.EMPTY)
        whenever(pricesFormatter.format(any(), any())).thenReturn(pricesVoTestInstance())
        whenever(reasonsFormatter.format(any<ProductOffer>())).thenReturn(
            recommendedReasonsVoTestInstance()
        )
        whenever(resourcesDataStore.getQuantityString(any(), any())).thenReturn("")
        whenever(moneyFormatter.formatPriceAsViewObject(any(), any())).thenReturn(MoneyVO.empty())

        // вызов тестируемых методов
        val offer = productOfferTestInstance(model = ModelInformation.testBuilder().offersCount(1).build())
        val viewObject = formatter.format(offer)

        // проверка
        assertThat(viewObject.offersCount, isEmptyString())
    }
}
```

### Проверки

#### Проверки значений

В качестве проверки объектов между собой следует использовать [Hamcrest](http://hamcrest.org/JavaHamcrest/tutorial)
Полный список проверок по ссылке

Пример
```kotlin
assertThat(provider.getRearrFactors(), hasItem(extraRearr))
```

#### Проверки вызовов

Если вы хотите проверить, что на ваших моках были вызванны методы, то нужно использовать [Mockito](https://www.javadoc.io/doc/org.mockito/mockito-core/2.7.10/org/mockito/Mockito.html)
Как было сказано выше, Mockito позволяет делать экземпляр объекта, за которым можно следить

Пример
```kotlin
// метод был вызван 2 раза
verify(stringSupplier, times(2)).invoke()
// метод не вызывался с любыми параметрами
verify(userProfilesUseCase, never()).forceSaveProfile(any())
// метод вызывался с любыми параметрами или null
verify(resolveDeeplinkUseCase, times(1)).resolve(anyOrNull())
// метод вызывался с любым 1 параметром и 2-ым параметром, равным targetFragment
verify(transaction).add(any(), eq(targetFragment))
// сложная проверка параметра
verify(cartItemFapiDataStore).addOffer(argThat { first().price.value.compareTo(primaryPrice.amount()) == 0 })
```

#### Проверка, что был выброшен Exception.
Над телом сценария нужно повесить аннотацию с классом ожидаемого эксепшена `@Test(expected = IllegalArgumentException::class)`
Если вам нужно проверить, что Exception был выброшен, но также проверить ещё что-то, то вышеописанный случай вам не поможет.
Нужно использовать Rule и на нём проверить, что Exception был. И также сделать другие проверки
```kotlin

    @get:Rule
    var thrown: ExpectedException = ExpectedException.none()

    @Test
    fun `Send event if scheme doesn't equals with uri scheme and source is push`() {
        val scheme = "some_scheme"
        val parser = SimpleDeeplinkParser(
            scheme,
            httpClient,
            authManager,
            getNavigationNodeUseCase,
            analyticsService,
            whiteMarketDeeplinkUseCase,
            liveStreamToggleManager
        )
        val uri = Uri.EMPTY
        val source = DeeplinkSource.PUSH_DEEPLINK

        thrown.expect(IllegalArgumentException::class.java)
        val deeplink = parser.parse(uri, source)
        verify(analyticsService).report(argThat<HealthEvent> { this.name() == HealthName.PUSH_DEEPLINK_UNKNOWN })
    }

```

### Параметризованные тесты

Если вам нужно проверить одну и ту же логику с большим количеством разных входных данных, то лучше написать параметризованный тест

1. Помечаем аннотацией ```@RunWith(Parameterized::class)``` класс теста
2. В конструкторе принимаем 2 параметра. Входные параметры и ожидаемый результат
3. Создаём статический метод, возвращающий список (размер списка - количество прогонов) массивов. В массиве первый элемент - входные данные, второй - ожидаемый результат

Пример
```kotlin
@RunWith(Parameterized::class)
class BundleGrouperTest(
    private val input: List<OrderItem>,
    private val expectedResult: List<OrderItem>
) {
    private val grouper = BundleGrouper()

    @Test
    fun `Check actual result equal to expected`() {
        val actualResult = grouper.groupOrderItemsByBundle(input)
        assertThat(actualResult, equalTo(expectedResult))
    }

    companion object {

        @Parameterized.Parameters
        @JvmStatic
        fun data(): Iterable<Array<*>> {
            return listOf(

                // 0
                arrayOf(
                    listOf(
                        orderItemTestInstance(skuId = "1", bundleId = "", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "2", bundleId = "", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "3", bundleId = "", isPrimaryBundleItem = false)
                    ),
                    listOf(
                        orderItemTestInstance(skuId = "1", bundleId = "", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "2", bundleId = "", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "3", bundleId = "", isPrimaryBundleItem = false)
                    )
                ),

                // 1
                arrayOf(
                    listOf(
                        orderItemTestInstance(skuId = "1", bundleId = "", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "2", bundleId = "1", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "3", bundleId = "1", isPrimaryBundleItem = true)
                    ),
                    listOf(
                        orderItemTestInstance(skuId = "1", bundleId = "", isPrimaryBundleItem = false),
                        orderItemTestInstance(skuId = "3", bundleId = "1", isPrimaryBundleItem = true),
                        orderItemTestInstance(skuId = "2", bundleId = "1", isPrimaryBundleItem = false)
                    )
                )
            )
        }
    }
}

```

### Как тестировать RxJava

Тестировать RxJava нужно и как можно больше, так как она иногда может преподносить сюрпризы или работать не так, как вы ожидаете

#### Основа
У класса Observable, Single, Completable есть метод `test()`, который возвращает **TestObserver**, на котором можно вызывать проверки
Также можно просто подписываться и проверять какую-то логику

```kotlin
    @Test
    fun `Valve with 0 buffer emit values when open`() {
        Flowable.fromIterable(listOf(1, 2, 3))
            .valve(Publishers.empty(), true, 0)
            .test()
            .assertNoErrors()
            .assertComplete()
            .assertValues(1, 2, 3)
    }

    @Test
    fun `Observable concatMapMaybe to empty maybe does not terminate source`() {
        Observable.just("")
            .concatWith(Observable.never())
            .concatMapMaybe { Maybe.empty<String>() }
            .test()
            .assertNotComplete()
    }

    @Test
    fun `Single created from emitter calls cancellation action on success`() {
        val cancellationAction = mock<Cancellable>()

        Single.create { emitter: SingleEmitter<String> ->
            emitter.setCancellable(cancellationAction)
            emitter.onSuccess("")
        }.subscribe()

        verify(cancellationAction).cancel()
    }
```

#### Многопоточность
- Переключение потоков выключается для тестов за счёт подмены Scheduler'ов
- Инжектить в презентеры нужно `presentationSchedulersMock()`

#### Тестирование со сдвигом по времени
Тестировать смещения по времени (delay, debounce, timer и т.п.) нужно через **TestScheduler**. Он позволяет двигать время

```kotlin
    @Test
    fun `retryWhen with timeout uses passed scheduler`() {
        val timerScheduler = TestScheduler()
        val counter = AtomicInteger(0)
        val observer = Observable.create<String> { emitter ->
            if (counter.getAndIncrement() < 2) {
                emitter.onError(RuntimeException())
            } else {
                emitter.onNext("")
                emitter.onComplete()
            }
        }
            .retryWhen {
                it.flatMap {
                    Observable.timer(2, TimeUnit.SECONDS, timerScheduler)
                }
            }
            .subscribeOn(Schedulers.trampoline())
            .test()

        observer.assertNotComplete()
            .assertNoErrors()
            .assertNoValues()

        timerScheduler.advanceTimeBy(4, TimeUnit.SECONDS)

        observer.assertResult("")
    }
```

### Как тестировать Android

Android-классы тестируются и подменяются с помощью [Robolectric](http://robolectric.org/)

Над тестируемым классом вешаем аннотации
- `@RunWith(RobolectricTestRunner::class)`
- `@Config(sdk = [Build.VERSION_CODES.P])`

Получить объект класса Context можно через
`val context = ApplicationProvider.getApplicationContext<Context>()`

Пример
```kotlin
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [Build.VERSION_CODES.P])
class AndroidFrameworkTests {

    /**
     * Поэтому в разметке compound-вьюх всегда нужнео использовать id с префиксами, иначе можно
     * случайно получить конфликт с id вьюх в Activity и долго ломать голову почему что-то работает
     * не совсем так как надо.
     */
    @Test
    fun `Find view by id traversal view hierarchy using depth search`() {
        val context = ApplicationProvider.getApplicationContext<Context>()
        val generatedId = View.generateViewId()
        val viewGroup = FrameLayout(context)
        val nestedViewGroup = FrameLayout(context)
        val nestedChild = TextView(context).apply { id = generatedId }
        val child = TextView(context).apply { id = generatedId }
        nestedViewGroup.addView(nestedChild)
        viewGroup.addView(nestedViewGroup)
        viewGroup.addView(child)

        val foundChild = viewGroup.findViewById<View>(generatedId)

        assertThat(foundChild, sameInstance(nestedChild as View))
    }
}
```

### Как тестировать корутины и Flow

// TODO

## Частые проблемы

Можно смело пополнять
