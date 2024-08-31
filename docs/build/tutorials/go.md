# Руководство по сборке кода на Go

## Прежде чем начать

Далее предполагается, что
- Локально имеется рабочая копия Аркадии полученная по [инструкции](https://docs.yandex-team.ru/devtools/intro/quick-start-guide).
- Разработчик знает в каком он проекте с точки зрения Аркадии (в какой папке верхнего уровня будет его код).
- Разработчик уже включён в одну из групп Арканум.

## Hello, World! {#hello_world}

В качестве первого проекта на `GO` в Аркадии (по традиции) рассмотрим `Hello, World!`. Каждый пакет в Аркадии должен находиться в собственной директории. Поэтому первым делом надо выбрать место внутри директории проекта и создать поддиректорию, в которой будет жить код пакета. В эту директорию кладём файлы с исходным кодом, а также файл ya.make.

Пример [devtools/examples/tutorials/go/example1/hello/](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example1/hello/).

Структура проекта:
```
devtools/examples/tutorials/go/example1/hello
├── main.go
└── ya.make
```

Код программы (`main.go`):

{% code '/devtools/examples/tutorials/go/example1/hello/main.go' lang='go' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/go/example1/hello/ya.make' %}

Каждый ya.make в Аркадии должен содержать владельца кода - того, к кому можно обратиться для исправления ошибок и одобрения изменений. Для этого служит макрос [OWNER](../manual/common/macros.md#owner). Желательно указывать сразу группу, чтобы владелец не терялся на время отпуска или при переходе в соседние подразделения.
Здесь [GO_PROGRAM](../manual/go/modules.md#go_program) говорит, что в этой директории будет собираться именно исполняемая программа (а не пакет для использования в других программах).
В макросе [SRCS](../manual/go/macros.md#srcs) перечисляются все исходные файлы необходимые для сборки программы или пакета.

{% note warning %}

Файлы для `CGO` перечисляются в макросе [CGO_SRCS](../manual/go/macros.md#cgo_srcs)

{% endnote %}

{% note info %}

В Аркадии есть замечательная утилита `yo` (`ya tool yo`), поддерживаемая GO комитетом. Подробнее об использование этой утилиты можно почитать [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yo/README.md). С помощью `yo` можно по директории проекта, содержащей исходные файлы, [автоматически сгенерировать `ya.make`](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/yo/README.md#avtomaticheskaya-generaciya-ya.make) файлы, описывающие сборку проекта (включая тесты).

{% endnote %}

Программа написана, её можно собрать:
```
$ cd <aracdia-root>
$ ya make <project-dir>
```
Поскольку в Аркадии принято правило _"одна директория - один модуль"_, то для сборки надо указывать директорию, где модуль (в данном случае программа) описан.
Если вы уже находитесь в этой директории, то путь можно не указывать.

{% note warning %}

Если не указать имя директории находясь в корне Аркадии, то будет сделана попытка собрать всю Аркадии. Это может быть ооочень долго и вряд ли вам такое может понадобиться.

{% endnote %}

Например:
```
$ ya make devtools/examples/tutorials/go/example1/hello
Ok
$ ls devtools/examples/tutorials/go/example1/hello
hello  main.go  ya.make
$ devtools/examples/tutorials/go/example1/hello/hello
Hello, World!
$
```

Или
```
$ cd devtools/examples/tutorials/go/example1/hello
$ ya make
$ ls
hello  main.go  ya.make
$ ./hello
Hello, World!
$
```

После окончания сборки в директории проекта появился исполняемый файл (а точнее символьная ссылка) `hello`. Это и есть наша первая программа на `GO`. Её можно запустить и увидеть вывод этой программы в терминале: `Hello, World!`.

{% note info %}

Символьная ссылка в директории проекта появится только для `Linux` и `Darwin`. При построении на `Windows` ссылка на артефакт сборки не создаётся внутри рабочей копии. Чтобы получить артефакт сборки воспользуйтесь дополнительными флагами `ya make`: `-o <output-dir>` или `-I <output-dir>`. ***Важно! не указывайте в качестве `<output-dir>` директорию внутри рабочей копии Аркадии.***

{% endnote %}

Имя программы совпадает с последним слогом пути директории, содержащей `ya.make` c описанием сборки этой программы.
Для программ/пакетов автоматически создаются тесты стиля и статической верификации кода (`fmt` и `vet`). Чтобы увидеть наличие проблем при оформлении кода нужно запустить сборку с флагом `-t`.
```
$ ya make -t
devtools/examples/tutorials/go/example1/hello <gofmt>
[good] main.go::gofmt [default-linux-x86_64-debug] (0.00 s)
Logsdir: devtools/examples/tutorials/go/example1/hello/test-results/gofmt/testing_out_stuff
Stderr: devtools/examples/tutorials/go/example1/hello/test-results/gofmt/testing_out_stuff/main.go.gofmt.err
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example1/hello

devtools/examples/tutorials/go/example1/hello <govet>
[good] hello.vet.txt::govet [default-linux-x86_64-debug] (0.09 s)
Logsdir: devtools/examples/tutorials/go/example1/hello/test-results/govet/testing_out_stuff
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example1/hello

Total 2 suites:
        2 - GOOD
Total 2 tests:
        2 - GOOD
Ok
```

## Пакет для Hello, World! {#go_library}

Допустим нам нужно создать какой-то пакет для использования в разных программах. В терминах Аркадийной системы сборки пакет - это [GO_LIBRARY](../manual/go/modules.md#go_library). Описание пакета не сильно отличается от программы:

Пример [devtools/examples/tutorials/go/example2/internal/greeting](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example2/internal/greeting/).

Структура проекта:
```
devtools/examples/tutorials/go/example2/internal/greeting
├── greeting.go
└── ya.make
```

- Путь от корня Аркадии до директории описания пакета с префиксом `a.yandex-team.ru/` будет формировать имя пакета (это правило должно выполняться для всех пакетов кроме пакетов из стандартной библиотеки и пакетов из корневой директории `vendor`, в которой располагаются весь внешний по отношению к Аркадии код на `GO` - third party)
- Как и в случае программы, все исходные файлы пакета должны быть указаны в макросе [SRCS](../manual/go/macros.md#srcs) или [CGO_SRCS](../manual/go/macros.md#cgo_srcs).
- Чтобы использовать пакет в исходном коде, расположенном в репозитории, не надо указать путь до него в макросе `PEERDIR` в описании сборки зависимого пакета или программы (это требуется только, если пакет используется в сгенерированном файле).

Код пакета (`greeting.go`):

{% code '/devtools/examples/tutorials/go/example2/internal/greeting/greeting.go' lang='go' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/go/example2/internal/greeting/ya.make' %}

Здесь [GO_LIBRARY](../manual/go/modules.md#go_library) говорит, что в директории проекта будет собираться пакет, который можно будет использовать в других пакетах или программах.

Также как и программу, пакет можно собрать командой `ya make` и проверить, что всё в порядке со стилем командой `ya make -t`.

**Пример**

```
$ ya make -t devtools/examples/tutorials/go/example2/internal/greeting
devtools/examples/tutorials/go/example2/internal/greeting <gofmt>
[good] greeting.go::gofmt [default-linux-x86_64-debug] (0.00 s)
Logsdir: devtools/examples/tutorials/go/example2/internal/greeting/test-results/gofmt/testing_out_stuff
Stderr: devtools/examples/tutorials/go/example2/internal/greeting/test-results/gofmt/testing_out_stuff/greeting.go.gofmt.err
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example2/internal/greeting

devtools/examples/tutorials/go/example2/internal/greeting <govet>
[good] greeting.a.vet.txt::govet [default-linux-x86_64-debug] (0.08 s)
Logsdir: devtools/examples/tutorials/go/example2/internal/greeting/test-results/govet/testing_out_stuff
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example2/internal/greeting

Total 2 suites:
        2 - GOOD
Total 2 tests:
        2 - GOOD
Ok
```

Пакет написан. Со стилем проблем нет. Но нам бы хотелось проверить его работоспособность.
Это можно сделать, написав программу с использованием пакета, или написав тесты. Об этом будет чуть ниже.

## Hello, World! с использованием пакета {#hello_world_with_deps}

У нас есть пакет [devtools/examples/tutorials/go/example2/internal/greeting](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example2/internal/greeting), который умеет правильно формировать приветствие. Давайте воспользуемся функцией [SayHello](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example2/internal/greeting/greeting.go?rev=7538110#L5) для формирования текста приветствия в программе Hello, World! [devtools/examples/tutorials/go/example2/hello](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example2/hello/).

Структура проекта:
```
devtools/examples/tutorials/go/example2
├── hello
│   ├── main.go
│   └── ya.make
├── internal
│   └── greeting
│       ├── greeting.go
│       └── ya.make
└── ya.make
```

Код обновлённого `Hello, World!`:

{% code '/devtools/examples/tutorials/go/example2/hello/main.go' lang='go' %}

Код описания сборки (`ya.make`):

{% code '/devtools/examples/tutorials/go/example2/hello/ya.make' %}

Как вы видите, код `ya.make` файла, описывающего сборку обновлённого `Hello, World!`, не изменился. `ya make` сам смог определить зависимый пакет (распарсив импорты исходного файла [main.go](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example2/hello/main.go?rev=7538110#L6)) `a.yandex-team.ru/devtools/examples/tutorials/go/example2/internal/greeting` и добавить его в сборочный граф для обновлённого `Hello, World!` - указывать явно `PEERDIR(devtools/examples/tutorials/go/example2/internal/greeting)` в `ya.make` файле не нужно.

## Тестируем пакет {#go_library_with_tests}

Давайте добавим тесты для функции формирования текста приветствия `SayHello` из пакета `greeting`.

Пример [devtools/examples/tutorials/go/example3/internal/greeting](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example3/internal/greeting/).

Структура проекта:
```
devtools/examples/tutorials/go/example3/internal/greeting
├── gotest
│   └── ya.make
├── greeting.go
├── greeting_test.go
└── ya.make
```

Код теста (`greeting_test.go`)

{% code '/devtools/examples/tutorials/go/example3/internal/greeting/greeting_test.go' lang='go' %}

Чтобы подключить тест в сборку мы добавили директорию с описания сборки теста [devtools/examples/tutorials/go/example3/internal/greeting/gotest](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example3/internal/greeting/gotest/) и внесли изменения в файл описания сборки тестируемого пакета.

Начнём с описания сборки тестов. Директория `gotest` содержит единственный файл (`ya.make`):

{% code '/devtools/examples/tutorials/go/example3/internal/greeting/gotest/ya.make' %}

Здесь [GO_TEST_FOR](../manual/go/modules.md#go_test_for) говорит, что в этой директории будет собираться тест для пакета из директории, указанной в параметре описания модуля `GO_TEST_FOR`. Этот модуль поддерживает вызовы "стандартных" тестовых макросов (`DATA`, `DEPENDS`, `SIZE`, `REQUIREMENTS`, `TAG` etc) для указания различных характеристик тестов и подвоза тестовых данных.

Теперь рассмотрим изменения в `ya.make` файле описания сборки тестируемого пакета `greeting`:

{% code '/devtools/examples/tutorials/go/example3/internal/greeting/ya.make' %}

Здесь [GO_TEST_SRCS](../manual/go/macros.md#go_test_srcs) говорит  о том, что `greeting_test.go` содержит внутренние (internal) тесты пакета `greeting`, а `RECURSE` после описания модуля говорит о том, что вместе с пакетом `greeting` нужно будет собрать проект в поддиректории `gotest`.

Теперь, если запустить команду `ya make -t` для построения пакета `greeting`, мы также построим тесты для этого пакета в директории `gotest` и запустим их на исполнение.
```
$ ya make -t devtools/examples/tutorials/go/example3/internal/greeting/
devtools/examples/tutorials/go/example3/internal/greeting/gotest <go_test>
[good] gotest::TestSayHello [default-linux-x86_64-debug] (0.00 s)
Log: devtools/examples/tutorials/go/example3/internal/greeting/gotest/test-results/gotest/testing_out_stuff/gotest.TestSayHello.log
Logsdir: devtools/examples/tutorials/go/example3/internal/greeting/gotest/test-results/gotest/testing_out_stuff
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example3/internal/greeting/gotest

devtools/examples/tutorials/go/example3/internal/greeting <gofmt>
[good] greeting.go::gofmt [default-linux-x86_64-debug] (0.00 s)
Logsdir: devtools/examples/tutorials/go/example3/internal/greeting/test-results/gofmt/testing_out_stuff
Stderr: devtools/examples/tutorials/go/example3/internal/greeting/test-results/gofmt/testing_out_stuff/greeting.go.gofmt.err
[good] greeting_test.go::gofmt [default-linux-x86_64-debug] (0.00 s)
Logsdir: devtools/examples/tutorials/go/example3/internal/greeting/test-results/gofmt/testing_out_stuff
Stderr: devtools/examples/tutorials/go/example3/internal/greeting/test-results/gofmt/testing_out_stuff/greeting_test.go.gofmt.err
------- GOOD: 2 - GOOD devtools/examples/tutorials/go/example3/internal/greeting

devtools/examples/tutorials/go/example3/internal/greeting <govet>
[good] greeting.a.vet.txt::govet [default-linux-x86_64-debug] (0.08 s)
Logsdir: devtools/examples/tutorials/go/example3/internal/greeting/test-results/govet/testing_out_stuff
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example3/internal/greeting

devtools/examples/tutorials/go/example3/internal/greeting/gotest <govet>
[good] gotest.vet.txt::govet [default-linux-x86_64-debug] (0.08 s)
Logsdir: devtools/examples/tutorials/go/example3/internal/greeting/gotest/test-results/govet/testing_out_stuff
------- GOOD: 1 - GOOD devtools/examples/tutorials/go/example3/internal/greeting/gotest

Total 4 suites:
        4 - GOOD
Total 5 tests:
        5 - GOOD
Ok
$
```

## Hello, World! с использованием CGO {#hello_world_with_cgo}
Давайте теперь рассмотрим как правильно описать сборку с `CGO`.
Пример [devtools/examples/tutorials/go/example4/hello](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example4/hello)

Структура проекта:
```
devtools/examples/tutorials/go/example4/hello
├── main.go
├── say_hello.cpp
├── say_hello.h
└── ya.make
```

В этом примере мы будем использовать код на `C++` для формирования текста приветствия для `Hello, World!`. Файлы `say_hello.h` и `say_hello.cpp` содержат объявление и реализацию функции `SayHello`, причём функция `SayHello` имеет `C` интерфейс и реализация написана на `C++`.
Файл `ya.make`, описывающий сборку проекта сильно отличается от первоначального:

{% code '/devtools/examples/tutorials/go/example4/hello/ya.make' %}

В вызове макроса [SRCS](../manual/go/macros.md#srcs) перечислен только файл `say_hello.cpp`. `main.go` - импортирует пакет `"C"` и поэтому он перечислен в вызове макроса [CGO_SRCS](../manual/go/macros.md#cgo_srcs). Мы добавили вызов макроса `USE_UTIL` (добавляет `PEERDIR` на `util` и `C++` runtime) так как реализация функции `SayHello` использует код из Аркадийного `util`.

{% note info %}

Если бы мы не использовали код из Аркадийного `util` достаточно было бы сделать вызов макроса `USE_CXX()`, чтобы подвезти на линковку `C++` runtime. Если бы реализация `SayHello` была написана на `C`, то вызов макроса `USE_CXX()` не потребовался.

{% endnote %}

## Hello, World! с использованием кодогенерации {#hello_world_with_codegen}

Теперь рассмотрим простой пример с кодогенерацией.

Пример [devtools/examples/tutorials/go/example5](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example5/hello)

Структура проекта:
```
devtools/examples/tutorials/go/example5/hello/
├── gen_main.py
└── ya.make
```

В этом примере мы генерируем код `Hello, World!` и затем используем сгенерированный код на `GO` в сборке.

Посмотрим на изменения в `ya.make` файле описания сборки проекта:

{% code '/devtools/examples/tutorials/go/example5/hello/ya.make' %}

В `ya.make` файле нет вызова макроса [SRCS](../manual/go/macros.md#srcs), но появились вызов макроса кодогереации `PYTHON` и вызов макроса `PEERDIR`. Посредством вызова макроса `PYTHON` генерируется исходный файл `main.go` на `GO`. Так `main.go` перечислен в параметрах макроса `PYTHON` с ключевым словом `OUT` и генерируемый файл имеет _известное_ расширение _.go_, то этот файл будет _автоматически_ учтён при сборке программы/пакета, как если бы он был явно перечислен в вызове макроса [SRCS](../manual/go/macros.md#srcs). В генерированном файле `main.go` используется пакет из стандартной библиотеки [fmt](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/examples/tutorials/go/example5/hello/gen_main.py?rev=7546645#L5). Так как `ya make` не может распарсить `main.go` файл на стадии генерации сборочного графа (этого файла нет в репозитории и он появится только на стадии исполнения сборочного графа), нам пришлось **явно** добавить зависимость на пакет `fmt`. Для этого мы использовали переменную [GOSTD](../manual/go/vars.md#gostd). Эта переменная определяет путь до стандартной библиотеки `GO`, используемой в сборке, от корня Аркадии.
