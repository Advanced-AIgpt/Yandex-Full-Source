## Как мне вызвать резолвер?

Для вызова резолвера необходимо описать `FapiContract` и передать его в `FapiContractProcessor`.
`FapiContract` включает в себя описание:

* Версии api
* Названия резолвера
* Параметров вызова
* Политики вызова
* Политики кеширования
* Алгоритма извлечения результата

### Пример Front API контракта
```kotlin
class ResolveUserFriendsContract(
    private val gson: Gson,
    private val userId: String
) : FapiContract<List<UserFriendDto>> {

    override val apiVersion = FapiVersions.V1

    override val resolverName = "resolveUserFriends"

    override fun configureCachePolicy(): FapiContractCachePolicy {
        return enableCache(5.minutes)
            .ignoreRegion()
            .configure()
    }

    override fun formatParametersJson(): String {
        return jsonObject { 
            "userId" % userId
        } toStringUsing gson
    }

    override fun createExtractor(): FapiExtractorDeclaration<List<UserFriendDto>> {
        return extractor {
            val resultPromise = extractResult<ResolverResult>(gson)
            val userProfilesPromise = extractUserProfiles(gson)

            strategy {
                val result = resultPromise.get()
                val userProfiles = userProfilesPromise.get()
                val friendIds = result.ids.orEmpty()
                return@strategy selectByIds(userProfiles, friendIds)
            }
        }
    }

    class ResolverResult(
        @SerializedName("result") val ids: List<String>?
    )
}
```

## Контекст запроса

Перед каждым запросом на сервер формируется клиентский контекст запроса. Контекст автоматически присоединяется к запросу. В контекст входят следующие аттрибуты:

`UserAgent` - агент приложения
`AppVersion` - версия приложения
`RearrFactors` - реарр флаги для модификации поведения бекендов
`Experiments` - набор экспериментов, в которые попал пользователь
`AuthToken` - токен авторизации в пространстве сервисов Яндекса
`Uuid` - уникальный идентификатор установки приложения
`Muid` - уникальный идентификатор пользователя в бекендах покупки
`SelectedRegion` - выбранный пользователем регион в приложении
`Clid`, `Mclid`, `Ymclid`, `Vid`, `DistrType` - вспомогательные идентификаторы для подсчета бизнес-метрик

## Аутентификация

Аттрибуты `AuthToken`, `Uuid`, `Muid` являются частью контекста запроса и автоматически присоединяются к запросу, поэтому авторизация работает из коробки. Никаких дополнительных действий при добавлении контракта не требуется.

## Форматирование параметров резолвера

Если вашему резолверу требуются входные параметры, необходимо переопределить метод `formatParametersJson` и сформировать валидную json-строку с нужными значениями параметров. Сделать это можно несколькими способами:

### Форматирование через шаблоны строк

```kotlin
class MyResolverContract(
    private val myParameterValue: String
) : FapiContract<..>() {

    override fun formatParametersJson(): String {
        return """{
            "myParameter": "$myParameterValue"
        }"""
    }
}
```

### Форматирование через dsl (рекомендуется)

```kotlin
class MyResolverContract(
    private val gson: Gson,
    private val myParameterValue: String
) : FapiContract<..>() {

    override fun formatParametersJson(): String {
        return jsonObject {
            "myParameter" % myParameterValue
        ) toStringUsing gson
    }
}
```

### Форматирование через объект

```kotlin
class MyResolverContract(
    private val gson: Gson,
    private val myParameterValue: String
) : FapiContract<..>() {

    override fun formatParametersJson(): String {
        return gson.toJson(MyParams(myParameterValue))
    }

    private class MyParams(
        @SerializedName("myParameter) val myParameter: String
    )
}
```

## Извлечение результата контракта

Извлечение результата контракта состоит из двух этапов. На первом этапе указывается модель результата резолвера, а так же объявляются коллекции, которые потребуются в дальнейшем. На втором этапе из коллекций извлекаются модели в соответствии с результатом резолвера. Для описания алгоритма извлечения результата необходимо переопределить метод `createExtractor`.
```kotlin
class UserFriendsContract(
    private val gson: Gson,
    private val userId: String
) : FapiContract<List<UserFriendDto>>() {

    override fun createExtractor(): FapiExtractorDeclaration<List<UserFriendDto>> {
        return extractor { // Начинаем описание алгоритма
            // Этап 1
            val resultPromise = extractResult<ResolverResult>(gson) // Создаем промис для результата резолвера
            val userProfilesPromise = extractUserProfile(gson) // Создаем промис для коллекции профилей

            // Этап 2
            strategy {
                val result = resultPromise.get() // Получаем результат
                val userProfiles = userProfilesPromise.get() // Получаем коллекцию профилей
                val friendIds = result.ids.orEmpty() // Получаем список идентификаторов профилей из результата
                return@strategy selectByIds(userProfiles, friendIds) // Выбираем профили по списку идентификаторов
            }
        }
    }

    class ResolverResult(
        @SerializedName("result") val ids: List<String>?
    )
}
```

А теперь обо всем по порядку. Метод `extractor` это начало описания экстрактора. Внутри блока `extractor` доступен DSL-контекст, который позволяет удобно извлекать результат резолвера и коллекции. Для извлечения результата резолвера используйте метод `extractResult`:
```kotlin
override fun createExtractor(): FapiExtractorDeclaration<..> {
    return extractor {
        val resultPromise = extractResult<ResolverResult>(gson)
        ...
    }
}
```

Для извлечения коллекций используйте методы расширения для типа `FapiExtractorContext`. Для каждой коллекции должен быть описан свой метод извлечения с указанием ключа коллекции и типа элементов. Коллекция представляет собой словарь вида `Map<String, T>` где `T` это тип элемента коллекции.
**Важно!** Избегайте двух методов для парсинга одной и той же коллекции. Это может привести к ошибкам во время парсинга.

```kotlin
// Описание метода извлечения коллекции
// UserFriendDto       - тип элемента коллекции
// "profiles"          - ключ коллекции
fun FapiExtractorContext.extractUserProfiles(
    gson: Gson
): FapiCollectionPromise<Map<String, UserFriendDto>> {
    return extractCollection("profiles", gson)
}

override fun createExtractor(): FapiExtractorDeclaration<..> {
    return extractor {
        ...
        val userProfilesPromise = extractUserProfiles(gson) // Использование метода извлечения коллекции
        ...
    }
}
```

Метод `strategy` это начало описания стратегии обработки результата резолвера. В этом блоке происходит сопоставление результата резолвера с содержимым коллекций. Внутри блока `strategy` доступен DSL-контекст, который позволяет удобно работать с коллекциями. Контекст предоставляет такие методы как `selectById`, `selectByIds`, `selectByCustomId`, `selectByCustomIds` и их опциональные аналоги.

```kotlin
override fun createExtractor(): FapiExtractorDeclaration<..> {
    return extractor {
        ...
        strategy {
            val result = resultPromise.get() // Получаем результат
            if (result.error != null) {
                // Обрабатываем ошибку в результате резолвера
                throw FapiExtractorException(result.error.toString())
            }
            val userProfiles = userProfilesPromise.get() // Получаем коллекцию профилей
            val friendIds = result.ids.orEmpty() // Получаем список идентификаторов профилей из результата
            return@strategy selectByIds(userProfiles, friendIds) // Выбираем профили по списку идентификаторов
        }
    }
}
```

## Обработка ошибок резолвера

По умолчанию метод `extractResult` выбрасывает исключение если в результате есть поле `error`. Если ошибка это ожидаемый результат работы резолвера, ее можно добавить в модель результата резолвера, а при вызове метода `extractResult` укажите флаг `throwOnError = false`.

```kotlin
class MyContractContract(
    private val gson: Gson
) : FapiContract<..>() {

    override fun createExtractor(): FapiExtractorDeclaration<..> {
        return extractor {
            val resultPromise = extractResult<ResolverResult>(gson, throwOnError = false)

            strategy {
                val result = resultPromise.get() // Получаем результат
                if (result.error != null) {
                    // Обрабатываем ошибку в результате резолвера
                    throw FapiExtractorException(result.error.toString())
                }
                ...
            }
        }
    }

    class ResolverResult(
        @SerializedName("result") val ids: List<String>?,
        @SerializedName("error") val error: MyErrorDto?
    )
}
```

## Настройка политики кеширования

Кеширование доступно только для контрактов, возращающих `Serializable` результат. Чтобы включить кеширование, переопределите метод `configureCachePolicy`. Для формирования политики кеширования воспользуйтесь методом `enableCache`. По умолчанию `enableCache` учитывает авторизацию, регион пользователя и реарр-флаги при формировании ключа кеша. Если вы хотите игнорировать эти свойства, укажите это явно при помощи методов `ignoreAuthentication`, `ignoreRegion` и `ignoreRearrFactors` соответственно. Так же по умолчанию кеш контракта привязан к параметрам резолвера, то есть для каждого уникального набора параметров будет свой результат в кеше. Вы можете добавлять кастомные параметры (например, значение тогла) при помощи метода `considerCustomParameter`.

```kotlin
class MyCacheableContract(
    private val isMyToggleEnabled: Boolean
) : FapiContract<MyResult>() {

    override fun configureCachePolicy(): FapiContractCachePolicy {
        return enableCache(10.minutes)
            .ignoreRegion()
            .considerCustomParameter("myToggleValue", isMyToggleEnabled.toString)
            .configure()
    }

    class MyResult(
        val number: Int,
        val string: String
    ) : Serializable {
        companion object {
            private const val serialVersionUID = 1L
        }
    }
}
```

## Изолированные резолверы

Если вам по какой-то причине нужно гарантировать выполнение контракта изолированно от других, задайте свойству `isolated` значение `true`. В таком случае ваш контракт выполнится отдельным запросом.

```kotlin
class MyIsolatedContract : FapiContract<..>() {

    override val isolated = true
}
```

## Здоровье Front API на android

### Дашборд в графане

Дашборд в графане для отслеживания состояния Front API [https://grafana.yandex-team.ru/d/SrLW7MlGz/market-android-fapi-client-health](https://grafana.yandex-team.ru/d/SrLW7MlGz/market-android-fapi-client-health)

### Ошибки исполнения контрактов

* Список ошибок [YQL Запрос](https://yql.yandex-team.ru/Operations/YFxH5gPTTrrBdOl6Me4wROKnDv59WKZxZjD9_NhWODY=)
  * Фильтрация по времени
  * Фильтрация по пользователю
  * Фильтрация по резолверу
* Количество ошибок [YQL Запрос](https://yql.yandex-team.ru/Operations/YFxG7QuEI1SguAUbxvYchB_5WYjeGh_B-mdGBEzcQKA=)
  * Группировка по типу (сервер/клиент)
  * Группировка по резолверу
* Динамика ошибок [YQL Запрос](https://yql.yandex-team.ru/Operations/YFxLUb94hqYIQc49x_VAnMzgehaXxvhZMz84rdJUJ9A=)
  * График количества ошибок

### Производительность контрактов

* Статистика работы кеша [YQL Запрос](https://yql.yandex-team.ru/Operations/YGLsfCyLNTG-B_coigUY_ZJMbPQLDTwgFHgmI7mwoHE=)
  * Фильтрация по резолверу
  * Фильтрация по времени
* Медиана времени исполнения контракта [YQL Запрос](https://yql.yandex-team.ru/Operations/YGYPTL94hqYISBc8LQ7qANHErXt-aI6tTVZsS7e2RDA=)
  * Фильтрация по времени
  * Фильтрация по кеш-хиту 
* Медиана времени исполнения запроса [YQL Запрос](https://yql.yandex-team.ru/Operations/YGLwgiyLNTG-B_xcbYeZfEJvVnqS8QlC3CYx69iFTzc=)
  * Фильтрация по резолверу
  * Фильтрация по времени
* Медиана времени парсинга ответа [YQL Запрос](https://yql.yandex-team.ru/Operations/YGLw2CyLNTG-B_zt7HV-FUw-aHqZ5FjF0lYprDsRlqM=)
  * Фильтрация по резолверу
  * Фильтрация по времени
