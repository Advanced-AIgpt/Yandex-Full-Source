# Тесты : запуск произвольных программ

Данный тип тестов позволяет выполнить **произвольную команду** и убедиться, что она успешно завершается.

{% note info %}

1. Примеры exec-тестов можно найти [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/test/tests/exectest).
2. Успешным считается завершение команды с кодом возврата 0.

{% endnote %}

Простое описание теста в **ya.make** выглядит так:

```yamake
OWNER(g:some-group)

EXECTEST() # Объявляем Exec-тест

RUN( # Команда, которую хотим выполнить
    cat input.txt
)

DATA( # Тестовые данные (здесь лежит input.txt)
    arcadia/devtools/ya/test/tests/exectest/data
)

DEPENDS( # Зависимость от других проектов (здесь лежат исходные коды cat)
    devtools/dummy_arcadia/cat
)

# Текущий каталог для теста (каталог с input.txt)
TEST_CWD(devtools/ya/test/tests/exectest/data)

END()
```

В общем случае в одном **ya.make** можно объявить несколько разных команд:

```yamake
OWNER(g:some-group)

EXECTEST()

RUN( # Первый тест
    NAME test-1 # Явное объявление имени теста
    echo "1"
)

RUN( # Второй тест
    NAME test-hello-world
    echo "Hello, world!"
)

END()
```

Каждое объявление макроса `RUN` - это отдельный тест. Тесты выполняются в том порядке, в котором перечислены в файле.

{% note warning %}

При параллельном запуске тестов в каждый из параллельно выполняемых потоков попадает только часть команд, указанных в **ya.make**. Поэтому не рекомендуется писать команды, результаты выполнения которых зависят от выполнения команд из вышестоящих макросов `RUN`.

{% endnote %}

В общем виде макрос `RUN` предоставляет множество других возможностей:

```yamake
OWNER(g:some-group)

EXECTEST()

RUN(
    NAME my-test # Имя теста
    ENV TZ=Europe/Moscow
    ENV LANG=ru_RU.UTF-8 # Переменные окружения
    echo "1" # Команда и ее флаги
    STDIN ${ARCADIA_ROOT}/my-project/filename.txt # Файл, который подается с stdin команде
    STDOUT ${TEST_CASE_ROOT}/test.out # Куда сохранить stdout команды
    STDERR ${TEST_CASE_ROOT}/test.err # Куда сохранить stderr команды
    CWD ${ARCADIA_BUILD_ROOT}/my-project # Рабочий каталог теста
    CANONIZE ${TEST_CASE_ROOT}/test.out # Путь до файла с эталонными данными (будет сохранен в Sandbox)
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/test.out # Путь до файла с эталонными данными (будет сохранен в подкаталог canondata)
    DIFF_TOOL my-project/tools/my-difftool/my-difftool # Путь до исполняемого файла, используемого для сравнения вывода теста с эталонными данными
)

DEPENDS(
    my-project/tools/my-difftool
)

END()
```

Для указания путей доступны следующие переменные:

Переменная | Описание
:--- | :---
`${ARCADIA_BUILD_ROOT}` | Корень сборочной директории
`${ARCADIA_ROOT}` | Корень единого репозитория
`${TEST_SOURCE_ROOT}` | Путь к каталогу, в котором находится ya.make-файл текущего теста
`${TEST_CASE_ROOT}` | Путь к каталогу с результатами текущего теста
`${TEST_WORK_ROOT}` | Путь к рабочему каталогу теста
`${TEST_OUT_ROOT}` | Путь к каталогу с результатами прохождения всех тестов (результаты каждого теста лежат во вложенном каталоге)

{% note alert %}

Программы, подключенные по `DEPENDS` не складываются в Аркадию, после исполнения теста там может оказаться симлинк, но во время исполнения теста программы там нет.
Не используйте `${ARCADIA_ROOT}` для указания пути до запускаемой программы. Бинари обычно доступны вообще без указания пути, но путь от корня Аркадии
(без указания корня) тоже сработает.

{% endnote %}

### To be documented

[https://wiki.yandex-team.ru/yatool/test/exectest/](https://wiki.yandex-team.ru/yatool/test/exectest/)
