# Тесты

Unit-тесты в Аркадии оформляются в виде отдельных целей со своим `ya.make`.
Для их запуска используется команда `ya make -t`.
Подробности есть в [документации ya][ya doc]; см. также: [`RECURSE_FOR_TESTS`].

Для написания тестов у нас имеется два фреймворка:
*unittest* (наша собственная разработка) и *gtest* (популярное решение от Google).
Также есть библиотека [`library/cpp/testing/common`],
в которой находятся полезные утилиты, не зависящие от фреймворка.

Поддержка gtest в Аркадии появилась недавно,
поэтому фреймворк не очень распространен.
Тем не менее, у него есть ряд преимуществ по сравнению с unittest:

- очень подробно выводится контекст, когда ломается тест;
- два семейства макросов для сравнения:
  `ASSERT` останавливает тест, `EXPECT` отмечает тест как проваленный,
  но не останавливает его, давая возможность выполнить остальные проверки;
- возможность расширять фреймворк с помощью механизма матчеров;
- можно использовать gmock (справедливости ради заметим,
  что gmock можно использовать и в unittest;
  однако в gtest он проинтегрирован лучше, а проекты уже давно объединены);
- можно сравнивать некоторые типы —
  и даже получать относительно понятное сообщение об ошибке —
  даже если не реализован `operator<<`;
- интеграция почти с любой IDE из коробки;
- фреймворк известен во внешнем мире, новичкам не нужно объяснять, как писать тесты;
- есть поддержка fixtures (в unittest тоже есть, но имеет существенные недостатки реализации);
- есть поддержка параметрических тестов
  (один и тот же тест можно запускать с разными значениями входного параметра);
- можно использовать в open-source проектах.

К недостаткам gtest можно отнести:

- отсутствие поддержки аркадийных стримов.
  Впрочем, для популярных типов из `util` мы сделали собственные реализации `PrintTo`;
- отсутствие макроса, проверяющего, что код бросает ошибку с данным сообщением.
  Мы сделали такой макрос сами, а также добавили несколько полезных матчеров;

Четких рекомендаций по выбору фреймворка для тестирования нет —
руководствуйтесь здравым смыслом, обсудите с командой, взвесьте все «за» и «против».
Учитывайте, что при смене типа тестов с unittest на gtest
потеряется история запуска тестов в CI.
Впрочем, если ваши тесты не мигают (а мы очень на это надеемся),
история вам и не нужна.

## Gtest

Подробная документация по gtest доступна
[в официальном репозитории gtest][gtest doc] и [gmock][gmock doc].
Этот раздел дает лишь базовое представление о фреймворке,
а также описывает интеграцию gtest в Аркадию.

Для написания тестов с этим фреймворком используйте цель типа `GTEST`.
Минимальный `ya.make` выглядит так:

```yamake
GTEST()

OWNER(...)

SRCS(test.cpp ...)

END()
```

Поддерживаются стандартные для Аркадии настройки тестов:
`TIMEOUT`, `FORK_TESTS` и прочие.
Подробнее про это написано в [документации ya][ya doc].

Внутри `cpp` файлов с тестами импортируйте `library/cpp/testing/gtest/gtest.h`.
Этот заголовочный файл подключит gtest и наши расширения для него.
Для объявления тестов используйте макрос `TEST`.
Он принимает два параметра: название группы тестов (test suite)
и название конкретного теста.

{% note info %}

В аркадии своя реализация функции `main`, реализовывать ее не нужно. Для кастомизации своих тестов можно воспользоваться хуками из `library/cpp/testing/hook`.

{% endnote %}

Пример минимального `cpp` файла с тестами:

```cpp
#include <library/cpp/testing/gtest/gtest.h>

TEST(BasicMath, Addition) {
    EXPECT_EQ(2 + 2, 4);
}

TEST(BasicMath, Multiplication) {
    EXPECT_EQ(2 * 2, 4);
}
```

Для выполнения проверок в теле теста используйте специальные макросы.
Они, в отличие от стандартных `Y_ASSERT` и `Y_VERIFY`,
умеют печатать развернутые сообщения об ошибках.
Например, `ASSERT_EQ(a, b)` проверит, что два значения равны;
в случае, если это не так, макрос отметит тест как проваленный и остановит его.

Каждый макрос для проверки имеет два варианта:
`ASSERT` останавливает тест, `EXPECT` — нет.
По-умолчанию используйте `EXPECT`,
так вы получите больше информации после запуска тестов.
Используйте `ASSERT` только если проверяете условие,
от которого зависит корректность дальнейшего кода теста:

```cpp
TEST(EtheriaHeart, Activate) {
    auto* result = NEtheria::Activate();

    // Если указатель нулевой, последующие проверки приведут к UB.
    ASSERT_NE(result, nullptr);

    EXPECT_EQ(result->Code, 0);
    EXPECT_EQ(result->ConnectedCrystals, 5);
    EXPECT_GE(result->MaxPower, 1.5e+44);
}
```

Полный список доступных макросов есть в
[официальной документации][gtest macros]
и [документации по продвинутым возможностям gtest][gtest advanced macros].

Также gtest позволяет добавить к каждой проверке пояснение.
Оно будет напечатано если проверка провалится.
Для форматирования пояснений используется `std::ostream`:

```cpp
TEST(LotrPlot, Consistency) {
    EXPECT_GE(RingIsDestroyed() - FrodoLeftHobbiton(), TDuration::Days(183))
        << "no, eagles were not an option because " << Reasons();
}
```

Для более сложных проверок предусмотрен механизм матчеров.
Например, проверим, что контейнер содержит элемент,
удовлетворяющий определенному условию:

```cpp
TEST(GtestMatchers, SetElements) {
    THashSet<TString> elements{"water", "air", "earth", "fire"};
    EXPECT_THAT(elements, testing::Contains("air"));
    EXPECT_THAT(elements, testing::Contains(testing::StrCaseEq("Fire")));
}
```

Наши [расширения][gtest custom matchers src] добавляют макрос для проверки сообщений в исключениях:

```cpp
TEST(YException, Message) {
    EXPECT_THROW_MESSAGE_HAS_SUBSTR(
        ythrow yexception() << "black garnet is not active",
        yexception,
        "not active");
```

Для тестирования кода, прекращающего выполнение приложения
(например, выполняющего `Y_ASSERT` или `Y_VERIFY`),
используйте макросы
`EXPECT_DEATH`, `EXPECT_DEBUG_DEATH`, `ASSERT_DEATH`, `ASSERT_DEBUG_DEATH`:

```cpp
TEST(Vector, AccessBoundsCheck) {
    TVector<int> empty{};
    EXPECT_DEBUG_DEATH(empty[0], "out of bounds");
}
```


## Unittest

Для написания тестов с этим фреймворком используйте цель типа `UNITTEST`.
Минимальный `ya.make` выглядит так:

```yamake
UNITTEST()

OWNER(...)

SRCS(test.cpp ...)

END()
```

Поддерживаются стандартные для Аркадии настройки тестов:
`TIMEOUT`, `FORK_TESTS` и прочие.
Подробнее про это написано в [документации ya][ya doc].

Внутри `cpp` файлов с тестами подключите `library/cpp/testing/unittest/registar.h`.

{% note warning %}

Не подключайте файл `library/cpp/testing/unittest/gtest.h` —
это deprecated интерфейс unittest, который пытается выглядеть как gtest.

{% endnote %}

Для объявления группы тестов используйте макрос `Y_UNIT_TEST_SUITE`.
Для объявления тестов внутри группы используйте макрос `Y_UNIT_TEST`.

Пример минимального `cpp` файла с тестами:

```cpp
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(BasicMath) {
    Y_UNIT_TEST(Addition) {
        UNIT_ASSERT_VALUES_EQUAL(2 + 2, 4);
    }
    
    Y_UNIT_TEST(Multiplication) {
        UNIT_ASSERT_VALUES_EQUAL(2 * 2, 4);
    }
}
```

Для выполнения проверок в теле теста используйте специальные макросы:

| Макрос | Описание |
| ------ | -------- |
| **UNIT_FAIL**(*M*) | Отметить тест как проваленный и остановить его. |
| **UNIT_FAIL_NONFATAL**(*M*) | Отметить тест как проваленный но не останавливать его. |
| **UNIT_ASSERT**(*A*) | Проверить, что условие *A* выполняется. |
| **UNIT_ASSERT_EQUAL**(*A*, *B*) | Проверить, что *A* равно *B*. |
| **UNIT_ASSERT_UNEQUAL**(*A*, *B*) | Проверить, что *A* не равно *B*. |
| **UNIT_ASSERT_LT**(*A*, *B*) | Проверить, что *A* меньше *B*. |
| **UNIT_ASSERT_LE**(*A*, *B*) | Проверить, что *A* меньше или равно *B*. |
| **UNIT_ASSERT_GT**(*A*, *B*) | Проверить, что *A* больше *B*. |
| **UNIT_ASSERT_GE**(*A*, *B*) | Проверить, что *A* больше или равно *B*. |
| **UNIT_ASSERT_VALUES_EQUAL**(*A*, *B*) | Проверить, что *A* равно *B*. В отличие от `UNIT_ASSERT_EQUAL`, воспринимает `char*` как null-terminated строку, а также печатает значения переменных с помощью `IOutputStream` при провале проверки. |
| **UNIT_ASSERT_VALUES_UNEQUAL**(*A*, *B*) | Проверить, что *A* не равно *B*. В отличие от `UNIT_ASSERT_UNEQUAL`, воспринимает `char*` как null-terminated строку, а также печатает значения переменных с помощью `IOutputStream` при провале проверки. |
| **UNIT_ASSERT_DOUBLES_EQUAL**(*E*, *A*, *D*) | Проверить, что *E* и *A* равны с точностью *D*. |
| **UNIT_ASSERT_STRINGS_EQUAL**(*A*, *B*) | Проверить, что строки *A* и *B* равны. В отличие от `UNIT_ASSERT_EQUAL`, воспринимает `char*` как null-terminated строку. |
| **UNIT_ASSERT_STRINGS_UNEQUAL**(*A*, *B*) | Проверить, что строки *A* и *B* не равны. В отличие от `UNIT_ASSERT_UNEQUAL`, воспринимает `char*` как null-terminated строку. |
| **UNIT_ASSERT_STRING_CONTAINS**(*A*, *B*) | Проверить, что строка *B* является подстрокой в *A*. |
| **UNIT_ASSERT_NO_DIFF**(*A*, *B*) | Проверить, что строки *A* и *B* равны. Печатает цветной diff строк при провале проверки. |
| **UNIT_ASSERT_EXCEPTION**(*A*, *E*) | Проверить, что выражение *A* выбрасывает исключение типа *E*. |
| **UNIT_ASSERT_NO_EXCEPTION**(*A*) | Проверить, что выражение *A* не выбрасывает исключение. |
| **UNIT_ASSERT_EXCEPTION_CONTAINS**(*A*, *E*, *M*) | Проверить, что выражение *A* выбрасывает исключение типа *E*, сообщение которого содержит подстроку *M*. |

У каждого макроса `UNIT_ASSERT` есть версия `UNIT_ASSERT_C`,
позволяющая передать пояснение к проверке. Например:

```cpp
UNIT_ASSERT_C(success, "call should be successful");
UNIT_ASSERT_GT_C(rps, 500, "should generate at least 500 rps");
```


## Mock

Для реализации mock-объектов мы используем [gmock].
Если вы используете gtest, gmock подключен автоматически.
Если вы используете unittest, для подключения gmock нужно добавить `PEERDIR`
на [`library/cpp/testing/gmock_in_unittest`]
и импортировать файл `library/cpp/testing/gmock_in_unittest/gmock.h`.

## Утилиты для тестирования:
Все функции и утилиты, которые не зависят от тестового фреймворка распологаются в директории [common](https://a.yandex-team.ru/arc_vcs/library/cpp/testing/common)

- Получение сетевых портов в тестах:
  Для получения сетевого порта в тестах необходимо использовать функцию `NTesting::GetFreePort()` из [network.h](https://a.yandex-team.ru/arc_vcs/library/cpp/testing/common/network.h), которая вернет объект - владелец для порта. Порт будет считаться занятым до тех пор, пока владелец жив. 
Во избежание гонок с выделением портов необходимо держать объект-владелец в течении всей жизни теста, или по крайней мере пока в тесте не запустится сервис, который вызовет `bind` на этом порту.

Пример использования:
```
#include <library/cpp/testing/common/network.h>

TEST(HttpServerTest, Ping) {
    auto port = NTesting::GetFreePort();
    auto httpServer = StartHttpServer("localhost:" + ToString(port));
    auto client = CreateHttpClient("localhost:" + ToString(port));
    EXPECT_EQ(200, client.Get("/ping").GetStatus());
}
```

- Функции для получения путей для корня аркадии и до корня билд директории в тестах.  
  Все функции расположены в файле [env.h](https://a.yandex-team.ru/arc_vcs/library/cpp/testing/common/env.h) с описанием.

## Хуки для тестов
Хуки позволяют выполнять различные действия при старте или остановке тестовой программы. 
Подробнее про использование можно прочитать в [README.md](https://a.yandex-team.ru/arc_vcs/library/cpp/testing/hook/README.md)


## Тесты с канонизацией вывода

Тесты с канонизацией вывода используются для регрессионного тестирования.
При первом запуске теста вывод тестируемой программы сохраняется,
при последующих запусках система проверяет, что вывод не изменился.

К сожалению, в C++ такие тесты в данный момент не поддерживаются.

За состоянием поддержки этой возможности можно следить в задаче [DEVTOOLS-1467].


## Параметры теста

Поведение тестов можно настраивать, передавая в них параметры.

При запуске тестов,
используйте ключ `--test-param` чтобы передать в тест пару ключ-значение.
Например: `ya make -t --test-param db_endpoint=localhost:1234`.

Для доступа к параметру из теста
используйте функцию `GetTestParam`:

```cpp
TEST(Database, Connect) {
    auto endpoint = GetTestParam("db_endpoint", "localhost:8080");
    // ...
}
```


## Метрики теста

Наш CI поддерживает возможность выгрузки из теста пользовательских метрик.
У каждой метрики есть название и значение — число с плавающей точкой.
CI покажет график значений метрик на странице теста.

Для добавления метрик
используйте функцию `testing::Test::RecordProperty` если работаете с gtest,
или макрос `UNIT_ADD_METRIC` если работаете с unittest.
Например, создадим метрики `num_iterations` и `score`:

{% list tabs %}

- Gtest

  ```cpp
  TEST(Solver, TrivialCase) {
      // ...
      RecordProperty("num_iterations", 10);
      RecordProperty("score", "0.93");
  }
  ```
  
  {% note warning %}
  
  `RecordProperty` принимает на вход целые числа и строки.
  Наш CI не умеет работать со строковыми значениями метрик,
  поэтому каждую строку он будет воспринимать как число.
  В случае, если строку не удастся преобразовать в число, тест упадет.
  
  {% endnote %}
  
- Unittest

  ```cpp
  Y_UNIT_TEST_SUITE(Solver) {
      Y_UNIT_TEST(TrivialCase) {
          // ...
          UNIT_ADD_METRIC("num_iterations", 10);
          UNIT_ADD_METRIC("score", 0.93);
      }
  }
  ```

{% endlist %}


[`RECURSE_FOR_TESTS`]: https://docs.yandex-team.ru/ya-make/manual/common/macros#recurse
[`library/cpp/testing/common`]: https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/testing/common
[`library/cpp/testing/gmock_in_unittest`]: https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/testing/gmock_in_unittest
[DEVTOOLS-1467]: https://st.yandex-team.ru/DEVTOOLS-1467
[ya doc]: https://docs.yandex-team.ru/ya-make/manual/tests/common
[gtest]: https://github.com/google/googletest
[gtest doc]: https://github.com/google/googletest/tree/master/googletest/docs
[gmock]: https://github.com/google/googletest/blob/master/googlemock/README.md
[gmock doc]: https://github.com/google/googletest/tree/master/googlemock/docs
[gtest macros]: https://github.com/google/googletest/blob/master/googletest/docs/primer.md#assertions
[gtest advanced macros]: https://github.com/google/googletest/blob/master/googletest/docs/advanced.md#more-assertions
[gtest custom matchers src]: https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/testing/gtest_extensions/matchers.h
