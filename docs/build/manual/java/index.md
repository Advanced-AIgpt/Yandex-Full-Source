# Описание сборки Java

## Идеология
В Аркадии для C++, Java, Go и Python используется единая система сборки `ya make`. Это позволяет использовать одни и те же инструменты и описание сборки для всех проектов в репозитории, даёт возможность удобного переиспользования кода между разными языками, позволяет форсировать принятые практики, к которым относятся зависимости по исходникам и фиксирование окружения. В таком подходе любое изменение в репозитории может изменить что угодно, и чтобы сборка и тестирование были быстрыми, они должны быть распределёнными.

В этой статье хотим подробнее рассказать про устройство Java-сборки.
## Prerequisites: установка Аркадийной jdk
```
cd arcadia/jdk && ya make && mkdir ~/jdk && tar xf jdk17.tar -C ~/jdk
```
Подключите в IDE jdk

## Java модули
Каждый проект в системе сборки состоит из одного или нескольких модулей. Каждый модуль имеет свою семантику, а также свой набор выходных артефактов. 
В каждом `ya.make` может быть описано не более одного модуля. Описание модуля начинается с типа модуля и заканчивается макросом `END()`. Модули могут зависеть друг от друга, что выражается макросом `PEERDIR()`. 
* [`JAVA_LIBRARY()`](modules.md#JAVA_LIBRARY) – модуль для описания java-библиотеки. Выходной артефакт – .jar.
* [`JAVA_PROGRAM()`](modules.md#JAVA_PROGRAM) – модуль для описания java-программы. Выходные артефакты: .jar, директория со всеми jar-ми для формирования classpath.
* [`JTEST()`](modules.md#JTEST) [`JUNIT5()`](modules.md#JUNIT5) – модули для описания java-тестов. Если запрошено, система сборки будет сканировать исходники модуля на наличие junit4/junit5 тестов и запускать их. Выходные артефакты: jar, директория с выхлопом тестов(если запрошен запуск тестов) – логи тестов, логи системы тестирвания, временные файлы тестов и т.д..
* [`JAVA_CONTRIB_PROXY`](modules.md#JAVA_CONTRIB_PROXY), [`JAVA_CONTRIB_PROGRAM`](modules.md#JAVA_CONTRIB_PROGRAM) и [`JAVA_CONTRIB`](modules.md#JAVA_CONTRIB) - модули описывающие внешние бибилотеки попадающие в сборку в предсобранном виде из sandbox или Аркадии.

Все java модули имеют еще один опциональный выходной артефакт – это jar с исходниками(`a make --sources`). Все java модули могут зависеть от `JAVA_LIBRARY`. `JTEST` может зависеть от `JAVA_LIBRARY` и `JAVA_PROGRAM`. Все остальные типы зависимостей, хотя и могут работать, семантически неверны, и мы советуем их не использовать.

## JAVA_SRCS
С помощью макроса `JAVA_SRCS()` указываются java-исходники и ресурсы. Макрос может содержаться в любом из четырех java модулей.
Ключевые слова:
* `SRCDIR x` – позволяет указать директорию `x`, относительно которой будет происходить поиск исходников по указанным паттернам. Если `SRCDIR` отсутствует, поиск исходников будет происходить относительно директории модуля.
* `PACKAGE_PREFIX x` – используется, если пути исходников относительно `SRCDIR` не совпадают с полными именами классов. Например, если все исходники модуля лежат в одном и том же package, можно не создавать директорию `package/name`, а просто положить исходники в `SRCDIR` и указать `PACKAGE_PREFIX package.name`.

**Пример**

{% cut "example/ya.make" %}
```
JAVA_PROGRAM()

JAVA_SRCS(SRCDIR src/main/java **/*)

END()
```
{% endcut %}

{% cut "example/src/main/java/ru/yandex/example/HelloWorld.java" %}
```java
package ru.yandex.example;

public class HelloWorld {
    public static void main(String[] args) {
        System.out.println("Hello, World!");
    }
}
```
{% endcut %}

**Пример**

{% cut "example/ya.make" %}
```
JAVA_PROGRAM()

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.example **/*.java)

END()
```
{% endcut %}

{% cut "example/HelloWorld.java" %}
```java
package ru.yandex.example;

public class HelloWorld {
    public static void main(String[] args) {
        System.out.println("Hello, World!");
    }
}
```
{% endcut %}

По умолчанию для компиляции используется `javac`. Макрос `USE_ERROR_PRONE()` заменяет его на ((http://errorprone.info/index Error Prone)).

Все java исходники компилируются с ключами `-g -encoding UTF-8`. Версию jdk/компилятора можно узнать командой `ya tool javac -version`.

## PEERDIR
Зависимость между модулями выражается макросом `PEERDIR`. Все java модули могут зависеть от `JAVA_LIBRARY`, а `JTEST` также от `JAVA_PROGRAM`.

**Пример**

{% cut "example/prog/ya.make" %}
```
JAVA_PROGRAM()

PEERDIR(example/lib)

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.example **/*.java)

END()
```
{% endcut %}

{% cut "example/prog/HelloWorld.java" %}
```java
package ru.yandex.example;

import ru.yandex.utils.Utils;

public class HelloWorld {
    public static void main(String[] args) {
        System.out.println(Utils.sayHello() + ", World!");
    }
}
```
{% endcut %}

{% cut "example/lib/ya.make" %}
```
JAVA_LIBRARY()

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.utils **/*.java)

END()
```
{% endcut %}

{% cut "example/lib/Utils.java" %}
```java
package ru.yandex.utils;

public class Utils {
    public static String sayHello() {
        return "Hello";
    }
}
```
{% endcut %}

### Использование внешних maven библиотек
Система сборки **никак не взаимодействует с maven**, но использование внешних библиотек из maven-репозиториев возможно. Для этого нужно сначала проимпортировать библиотеку отдельной утилитой [`ya maven-import`](commands.md#ya-maven-import)

### Classpath
Мы не выделяем для каждого модуля отдельный compile/runtime/test classpath. Описываемые макросом `PEERDIR` зависимости не имеют типа. Для каждого модуля мы составляем один classpath. Этот classpath выступает как compile-classpath для компиляции исходников модуля. Если `JAVA_PROGRAM` запускается макросом `RUN_JAVA_PROGRAM`, classpath этого `JAVA_PROGRAM` выступает как runtime-classpath. Если запускаются тесты, classpath модуля `JTEST` или `JUNIT5` выступает как test-classpath.

В первом приближении classpath модуля – это полное транзитивное замыкание модуля по зависимостям. То есть, это зависимости модуля, зависимости этих зависимостей и т.д.. В так составленный classpath могут попасть разные версии одной и той же библиотеки из `contrib/java`. В таком случае мы выбираем одну версию, из-за чего classpath уже не является простым транзитивным замыканием. Для более гибкого управления classpath мы поддерживаем макросы `EXCLUDE` и `DEPENDENCY_MANAGEMENT`. Подробней о разрешении конфликтов версий можно прочитать в [`разделе посвящённом управлению зависимостями`](dependencies.md)

### Анализ classpath
Чтобы понимать, какая библиотека откуда приехала, почему выбралась версия библиотеки X, а не Y, почему библиотека A стоит в classpath раньше, чем B и т.д., есть 3 полезные команды: [`ya java classpath`](commands.md#ya-java-classpath), [`ya java test-classpath`](commands.md#ya-java-test-classpath) и [`ya java dependency-tree`](commands.md#ya-java-dependency-tree)

## Интеграция с cpp/python сборкой
Из-за особенностей реализации многие полезные макросы, работающие в cpp/python-модулях, не работают внутри java-модулей. В частности, не получится использовать `PYTHON`, `RUN_PROGRAM` для кодогенерации, `FROM_SANDBOX` для загрузки ресурсов из sandbox. Тем не менее, java-модули могут взаимодействовать с cpp/python-модулями.
* **JNI** – из любого java-модуля можно поставить `PEERDIR` на `DLL` или `DLL_FOR`. В результате динамическая библиотека попадет в `java.library.path` при запуске тестов, а также попадет в выходную директорию со всеми runtime-зависимостями для `JAVA_PROGRAM`.
* **SWIG** – из любого java-модуля можно поставить `PEERDIR` на `DLL_JAVA`. В `SRCS` у `DLL_JAVA` указаны swig-исходники. Полученная из `DLL_JAVA` динамическая библиотека обрабатывается так же, как и в случае `PEERDIR` на `DLL`. Полученный из `DLL_JAVA` jar попадает в classpath. [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/test_swig_java).
* **PROTO_LIBRARY** – из любого java-модуля можно поставить `PEERDIR` на `PROTO_LIBRARY`. `PROTO_LIBRARY`, а также все достижимые из нее `PROTO_LIBRARY` попадут в classpath в виде jar со скомпилированными сгенерированными классами.
* **PY2TEST** - артефакты java-модулей можно использовать в тестах `PY2TEST`. [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/java_in_pytest?rev=2999154).

## Кодогенерация, RUN_JAVA_PROGRAM
Помимо `PROTO_LIBRARY`, отвечающего за кодогенерацию `proto`, в java-модуле можно делать произвольную кодогенерацию через `RUN_JAVA_PROGRAM`. С помощью этого макроса можно запустить указанный main из указанного `JAVA_PROGRAM` с указанными аргументами. **Запуск maven-плагинов as is не поддерживается**, но руками, выделив полезную логику, maven-плагин часто удается превратить в `JAVA_PROGRAM` с понятным cli-интерфейсом, который далее можно использовать в `RUN_JAVA_PROGRAM`.

### BUILD_ROOT
Чтобы правильно писать `RUN_JAVA_PROGRAM` нужно понимать, что такое `BUILD_ROOT`. Перед тем, как начать сборку, `ya make` генерирует сборочный граф. Каждая нода этого графа имеет:
* `deps` – список зависимых нод
* `cmds` – список команд для выполнения
* `outputs` – список выходных файлов

Далее `ya make` выполняет ноды графа в порядке топологической сортировки:
1. Выполнение ноды начинается только тогда, когда все `deps` уже выполнены
2. Перед выполнением ноды для нее создается временная директория aka `BUILD_ROOT`, куда доставляются `outputs` каждой ноды из `deps`
3. Выполняется нода, т.е. последовательно выполняются все ее `cmds`. Каждая команда использует `outputs` зависимых нод и файлы из Аркадии, чтобы создать новые файлы в `BUILD_ROOT`. В результате выполнения команд в `BUILD_ROOT` должны появиться все заявленные в ноде `outputs`. Эти файлы затем могут быть использованы в какой-то другой ноде или являться конечными артефактами сборки.

Для нас важно, что в случае `RUN_JAVA_PROGRAM` сгенерированные исходники не попадают в Аркадию ни в каком виде. Все выходные файлы/директории должны быть в `BUILD_ROOT`. Для того, чтобы затем использовать директорию со сгенеренными файлами, нужно в `SRCDIR` для `JAVA_SRCS` указать директорию из `BUILD_ROOT`. Для этого используются переменные
* `ARCADIA_BUILD_ROOT` – это `BUILD_ROOT`
* `BINDIR` – будучи использованный в `path/to/module/ya.make` эквивалентна `${ARCADIA_BUILD_ROOT}/path/to/module`

**Пример**

{% cut "example/prog/ya.make" %}
```
JAVA_PROGRAM()

RUN_JAVA_PROGRAM(
    ru.yandex.gen.GenHelloWorld ${BINDIR}/generated
    OUT_DIR ${BINDIR}/generated
    CLASSPATH example/codegen_prog
)

JAVA_SRCS(SRCDIR ${BINDIR}/generated **/*.java)

END()
```
{% endcut %}

{% cut "example/codegen_prog/ya.make" %}
```
JAVA_PROGRAM()

JAVA_SRCS(PACKAGE_PREFIX ru.yandex.gen GenHelloWorld.java)

END()
```
{% endcut %}

{% cut "example/codegen_prog/GenHelloWorld.java" %}
```java
package ru.yandex.gen;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;

public class GenHelloWorld {
    public static void main(String[] args) {
        String outputDir = args[0];
        new File(outputDir).mkdirs();

        String outputFileName = outputDir + "/HelloWorld.java";
        try {
            PrintWriter writer = new PrintWriter(outputFileName, "UTF-8");
            writer.println("public class HelloWorld {public static void main(String[] args) {System.out.println(\"Hello, World!\");} }");
            writer.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
```
{% endcut %}

### Синтаксис
```
RUN_JAVA_PROGRAM(
    jvm_arg1 jvm_arg2 ...
    MainClass arg1 arg2 ...
    IN_DIR dir1 dir2 ...
    IN file1 file2 ...
    OUT_DIR dir1 dir2 ...
    CWD cwd
    CLASSPATH path/to/java/program
)
```

Ключевое слово `CLASSPATH` обязательно. Все остальные ключевые слова: `IN_DIR`, `IN`, `OUT_DIR`, `CWD` – опциональны. Команда запуска состоит из части `$(JDK)/bin/java -classpath <classpath>`, которая генерируется автоматически, и части `jvm_arg1 jvm_arg2 ... MainClass arg1 arg2 ...`, которая берется из `ya.make`.

Описание ключевых слов
* `CLASSPATH` – путь до `JAVA_PROGRAM` чей classpath будет использоваться для запуска.
* `IN` – список входных файлов в Аркадии
* `IN_DIR` – список входных директорий в Аркадии
* `OUT_DIR` – список выходных директорий в `BUILD_ROOT`. Во избежании путаницы рекомендуется использовать пути, начинающиеся явно с `BINDIR` или `ARCADIA_BUILD_ROOT`. Например, передать программе как аргумент директорию `${BINDIR}/generated`, объявить эту директорию выходной, сказав `OUT_DIR ${BINDIR}/generated`, и использовать ее как `JAVA_SRCS(SRCDIR ${BINDIR}/generated **/*)`
* `CWD` – директория выполнения программы в Аркадии или в `BUILD_ROOT`

Очень часто значения `IN`, `IN_DIR`, `OUT_DIR` совпадают с какими-либо аргументами программы. Это происходит, потому что указывая какую-то директорию в аргументах, мы сообщаем самой программе о том, куда сложить результаты, а указывая ее повторно в `OUT_DIR`, мы сообщаем системе сборки, что данная директория появится после выполнения программы, и ее следует поместить в `outputs` ноды, чтобы потом переиспользовать ее в ноде компиляции. Это необходимое дублирование.

## Тестирование
Полное описание `JTEST`, `JUNIT5` [здесь](https://wiki.yandex-team.ru/yatool/test/#testynajava).
Дополнительно можно подключать [codestyle](https://wiki.yandex-team.ru/yatool/test/#proverkijavacodestyle) тесты, [classpath-clash](https://wiki.yandex-team.ru/yatool/test/#proverkijavaclasspath) тесты.

## Экспорт в maven
Есть возможность заливать собранные в Аркадии артефакты в указанный maven-репозиторий. Для этого в `ya make` есть ключи `--maven-export`, `--version`, `--deploy`, `--repository-id`, `--repository-url`, `--settings`. Обычно никто напрямую их не использует. Вместо этого настраивают автоматическую заливку в артефакторий на каждый коммит, затрагивающий проект.

Для этого нужно:
* Создать тест в testenv (пример из [iceberg](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/deploy/DeployIceberg.yaml)). Этот тест гарантирует, что на каждый коммит в [ObservedPaths](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/deploy/DeployIceberg.py?rev=2911232#L15 ) в sandbox в рамках задачи [ARCADIA_PY_SCRIPT_RUNNER](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/deploy/DeployIceberg.py?rev=2911232#L11) будет запускаться указанный [скрипт](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/deploy/DeployIceberg.py?rev=2911232#L20). Этот скрипт и занимается сборкой/заливкой артефактов. После создания нужно [добавить этот тест в проект](/TestEnvironment/КакначатьработатьсSandBoxиTestEnv/#sozdaniebazy).
* Реализовать скрипт, который запускает `ya make --maven-export ...`. Обычно эти скрипты сводятся к запуску [devtools/maven-deploy/deploy.py](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/maven-deploy/deploy.py). Пример [iceberg/deploy.py](https://a.yandex-team.ru/arc/trunk/arcadia/iceberg/deploy.py).

Пример того, как выглядят артефакты в maven репозитории: [iceberg-bolts](http://artifactory.yandex.net/artifactory/yandex_media_releases/ru/yandex/iceberg-bolts/).

## Поддержка IDE
По аркадийному описанию сборки команда `ya ide idea` может сгенерировать проект для IDEA.
* `--project-root, -r` — директория, где будут лежать все необходимые для проекта файлы. Это библиотеки из `contrib/java` с исходниками, все `PROTO_LIBRARY` в собранном виде, все `DLL` в собранном виде, проектные файлы `.ipr` и `.iws`.
* `--iml-in-project-root` — по умолчанию проектные `.iml` файлы генерируются в директориях самих java-модулей. Это гарантирует, что тесты модуля будут запускаться в директории модуля. Флаг `--iml-in-project-root` заставляет `ya ide idea` сгенерировать `.iml` в project-root.
* `--local, -l` — по умолчанию каждый java-модуль, имеющий исходники, открывается в IDEA как модуль, все остальное - как библиотека. Часто по `PEERDIR` приезжает много модулей, и проект становится громоздким. Флаг `--local` заставляет `ya ide idea` сгенерировать такой IDEA проект, что только исходные таргеты в нем будут модулями, а остальные модули будут собраны, и попадут в проект в виде библиотек.

Команда `ya ide idea` переводит описание сборки с языка `ya.make` на язык `.iml`, `.ipr`, понятный системе сборки IDEA. Это разные системы сборки, и не всегда получается добиться идентичного результата в IDEA. Сейчас известно две проблемы, когда runtime/test classpath в IDEA может отличаться от `ya make`. Первая проблема решается включением нашего [IDEA-плагина](https://clubs.at.yandex-team.ru/arcadia/12067). Вторая пока не решена, но люди натыкаются на нее редко.
1. Для формирования runtime-classpath модуля, IDEA сама транзитивно замыкает все compile и runtime зависимости модуля. Из-за этого в runtime-classpath модуля может приехать несколько версий одной и той же библиотеки, хотя в `ya make` такое невозможно. Чтобы отключить это поведение, нужно просто включить наш плагин. Команда `ya ide idea` после каждого запуска пишет путь до плагина.
2. В IDEA подразумевается, что и сам модуль и его тесты описаны в одном и том же `.iml` файле. В нашей системе сборки `JTEST` - это отдельный модуль, описанный в отдельном `ya.make`, причем он не обязательно привязан к какому-то одному java-модулю. При генерации проекта мы пытаемся прикрепить `JTEST` к какому-либо конкретному `JAVA_PROGRAM` или `JAVA_LIBRARY`. Для этого мы последывательно откусываем basename от пути до `JTEST` и проверяем, не попался ли java-модуль. То есть, если модуль лежит в `path/to/module`, а его тесты в `path/to/module/ut`, то мы правильно ассоциируем тесты с модулем. При этом в IDEA получится один модуль, для которого все зависимости из `path/to/module` – это compile-зависимости, а все зависимости из `path/to/module/ut` – это test-зависимости. При запуске тестов в IDEA compile-зависимости также попадут в test-classpath, из-за чего полный test-classpath IDEA-модуля может отличаться от classpath `JTEST` из `path/to/module/ut`.

### Поддержка PROTO_LIBRARY в IDEA
На каждую ##PROTO_LIBRARY## кроме classes.jar генерируется также sources.jar (исходники), чтобы была возможна кодонавигация. Некоторые исходники бывают слишком большими (лимит для индексации по умолчанию 2.5 MB) IDEA их не индексирует и игнорирует при попытке связать .class с .java - в итоге пытается генерировать исходный код декомпилятором (чем подвешивает IDEA на несколько секунд). 
Лимит можно расширить, поправив конфиг идеи:
* Меню Help -> Edit Custom Properties ...
в открывшемся файле написать:
* `idea.max.intellisense.filesize=10000`
размер - в килобайтах, 10000 для примера. После этого надо переиндексировать проект:
* Меню File -> Invalidate Caches / Restart ...

## Uberjar
Uberjar представляет собой единый all-in-one jar-архив включающий в себя все нужные ему (достижимые по PEERDIR) java зависимости. Также поддержана возможность переноса классов внутри архива в другой package (аналогично тому, как это делает [maven-shade-plugin](http://maven.apache.org/plugins/maven-shade-plugin/)). Для того, чтобы собирать uberjar, нужно в модуль `JAVA_PROGRAM` добавить макрос `UBERJAR`.
Для настройки содержимого архива можно использовать следующие макросы:
* `UBERJAR_HIDING_PREFIX` префикс для перемещаемых (shade) классов (по-умолчанию все остается на своих местах)
* `UBERJAR_HIDE_EXCLUDE_PATTERN` шаблон для классов, которые перемещать не надо (по-умолчанию исключений нет)
* `UBERJAR_PATH_EXCLUDE_PREFIX` префикс для путей, которые не должны попасть в jar-архив (по-умолчанию попадают все)
Пример использования этих макросов можно посмотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/test_uberjar/projectMain/ya.make).
