# Тесты на Java

Для Java [поддерживаются](https://docs.yandex-team.ru/devtools/test/intro#framework) [JUnit](https://junit.org/) версий 4.х и 5.х.

## Тесты на JUnit 4 { #junit4 }

Запуск тестов на JUnit 4 описывается макросом `JTEST()`:

```yamake
OWNER(g:my-group)

JTEST() # Используем JUnit 4

JAVA_SRCS(SRCDIR java **/*) # Где искать исходные коды тестов
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    # Сюда же необходимо добавить зависимости от исходных кодов вашего проекта
    contrib/java/junit/junit/4.12 # Сам фреймворк Junit 4
    contrib/java/org/hamcrest/hamcrest-all # Можно подключить набор Hamcrest матчеров
)

JVM_ARGS( # Необязательный набор флагов, передаваемых JVM
    -Djava.net.preferIPv6Addresses=true
    -Djava.security.egd=file:///dev/urandom
    -Xms128m
    -Xmx256m
)

SYSTEM_PROPERTIES( # Необязательный набор значений, которые нужно положить в Java system properties. Эти значения переопределяют те, что были переданы в JVM_ARGS при помощи -D.
    key1 val1
    key2 val2
    FILE app.properties # Положить содержимое *.properties или *.xml файла
    FILE config.xml
)


END()
```

{% note info %}

Примеры продвинутой работы с Java system properties можно посмотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/test_java_properties). Поддерживаются различные `${name}` подстановки и загрузка данных из XML файлов.

{% endnote %}


Код тестов совершенно стандартный, например:

```java
package ru.yandex.devtools.test;

import org.junit.Test;
import static org.junit.Assert.assertEquals;

public class MathsTest {

    @Test
    public void testMultiply() {
        assertEquals(2 * 2, 4);
    }

}
```

## Тесты на JUnit 5 { #junit5 }

Запуск тестов на JUnit 5 отличается только набором зависимостей и используемым макросом `JUNIT5`:

```yamake
OWNER(g:my-group)

JUNIT5() # Используем JUnit 5

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/contrib/java/org/junit/junit-bom/5.7.1/ya.dependency_management.inc)
PEERDIR(
    # Сюда же необходимо добавить зависимости от исходных кодов вашего проекта
    contrib/java/org/junit/jupiter/junit-jupiter # Сам фреймворк Junit 5
    contrib/java/org/hamcrest/hamcrest-all # Набор Hamcrest матчеров
)

END()
```

Пример теста:

```java
package ru.yandex.devtools.test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import org.junit.jupiter.api.Test;

class MathsTest {

    @Test
    void multiplication() {
        assertEquals(2 * 2, 4);
    }

}


```

## Доступ к зависимостям { #dependencies }

Если вы используете в своем **ya.make** [зависимости](https://docs.yandex-team.ru/devtools/test/dependencies), то обращаться к таким данным в коде теста нужно так:

```java
package ru.yandex.devtools.test;

import org.junit.jupiter.api.Test;
import ru.yandex.devtools.test.Paths;

class ReadFileTest {

    @Test
    void read() {
        // Путь до файла из DATA("my-project/tests/data")
        String testFilePath = Paths.getSourcePath(
            "my-project/tests/data/test.file"
        );

        // Путь до Sandbox-ресурса
        String sandboxResourcePath = Paths.getSandboxResourcesRoot() + "/file.txt";

        // Путь до файла из DEPENDS("my-project/tests/some-tool")
        String binaryFilePath = Paths.getBuildPath(
            "my-project/tests/some-tool/my-tool"
        );

        // ...
    }

}
```

## Доступ к параметрам { #parameters }

Для того, чтобы получить в коде значения [параметров](https://docs.yandex-team.ru/devtools/test/manual#parameters):

```java
package ru.yandex.devtools.test;

import org.junit.jupiter.api.Test;
import ru.yandex.devtools.test.Params;

class ReadParametersTest {

    @Test
    void read() {
        // Значение параметра my-param
        String myParamValue = Params.params.get("my-param");

        // ...
    }

}
```

## Проверка classpath { #classpath-check }

Для тестов на Java возможно включить автоматическую проверку на наличие нескольких одинаковых классов в [Java Classpath](https://en.wikipedia.org/wiki/Classpath). В проверке участвует не только имя класса, но и хэш-сумма файла с его исходным кодом, так как идентичные классы из разных библиотек проблем вызывать не должны. Для включения этого типа тестов в ya.make файл соответствующего проекта нужно добавить макрос `CHECK_JAVA_DEPS(yes)`:

```yamake
OWNER(g:my-group)

JTEST()

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

CHECK_JAVA_DEPS(yes) # Включаем проверку classpath

END()

```

## Статический анализ { #lint }

На все исходные тексты на Java, которые подключены в секции `JAVA_SRCS` файла **ya.make**, включён статический анализ. Для проверки используется утилита [checkstyle](https://checkstyle.org/). Поддерживается два уровня проверок: **обычный** и **расширенный** (extended). В расширенном режиме выполняется большее количество проверок. Есть возможность полностью отключить статический анализ.

```yamake
OWNER(g:my-group)

JTEST()

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

# Используйте один из следующих макросов:
LINT() # Включить статический анализатор
LINT(extended) # Включить статический анализатор в расширенном режиме (больше проверок)
NO_LINT() # Отключить статический анализатор

END()
```

{% note info %}

Конфигурационные файлы для статического анализа расположены [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/jstyle-runner/java/resources).

{% endnote %}

### To be documented

```
JTEST/JUNIT5
```

[https://wiki.yandex-team.ru/yatool/test/#java](https://wiki.yandex-team.ru/yatool/test/#java)
[https://wiki.yandex-team.ru/yatool/test/javacodestyle/](https://wiki.yandex-team.ru/yatool/test/javacodestyle/)
