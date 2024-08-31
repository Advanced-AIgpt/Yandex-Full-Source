# Разработка в Аркадии в Microsoft Visual Studio Code

## VSCode Yandex Plugin
Для работы с VSCode в Аркадии существует [расширение](/devtools/intro/quick-start-guide#vscode-plugin-setup) реализующее поддержку многих операций с [Arc VCS](/arc/) из редактора, а также интеграцию с Yandex.Paste и подсветку ya.make файлов.

## C++
Поддержка автодополнения и навигации для C++ проектов, использующих систему сборки [ya make](/ya-make/usage/ya_make/) требует предварительной настройки.

`ya ide vscode-clangd` - утилита, генерирующая VSCode Workspace, совместимый с расширением [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd).

Для переданного проекта утилита:

- выполняет кодогенерацию;
- генерирует базу команд сборки, которая будет использована при индексации;
- ограничивает отслеживание изменений файлов проекта (на Linux отключает полностью, т.к. встроенная поддержка inotify пожирает ресурсы);
- добавляет задачи для кодогенерации, выполнения сборки и перегенерации воркспейса;
- добавляет отладочные конфигурации для приложений и тестов проекта;
- настраивает линтинг `clang-tidy`.

### Опции запуска
`ya ide vscode-clangd [OPTION]... [TARGET]...`

`TARGET ...` - список путей проекта которые нужно включить в workspace и индексацию;

- ` -P=PROJECT_OUTPUT, --project-output=PROJECT_OUTPUT` - директория, в которой будет создан воркспейс. По умолчанию - текущая директория;
- `-W=WORKSPACE_NAME, --workspace-name=WORKSPACE_NAME` - название воркспейса. По умолчанию берётся имя директории `PROJECT_OUTPUT`.
- `--no-codegen` - не запускать кодогенерацию при создании воркспейса;
- `--use-arcadia-root` - использовать в качестве workspaceFolder корень аркадии (иначе будет использованы директории проекта);
- `--files-visibility=FILES_VISIBILITY` - в сочетании с `--use-arcadia-root` позволяет ограничить список директорий для отображения и поиска. `FILES_VISIBILITY` принимает значения:
    - `targets-and-deps` - отображать директории проекта и зависимостей (default);
    - `targets` - только директории проекта;
    - `all` - не ограничивать видимость.
- `--output=OUTPUT_ROOT` - директория для результатов кодогенерации (по-умолчанию: `$PROJECT_OUTPUT/.vscode/.build`);
- `-j=BUILD_THREADS` - количество тредов для индексации, кодогенерации, сборки. По умолчанию равно количеству логических процессоров на машине;
- `--setup-tidy` - настроить линтинг с `clang-tidy` ([подробнее](/ya-make/manual/tests/style#clang_tidy));
- `--make-args=YA_MAKE_EXTRA` - дополнительные аргументы запуска ya make;
- `-t, --tests` - создать отладочные конфигурации для тестов и подготовить тестовые окружения.


В `PROJECT_OUTPUT` будет создан файл воркспейса `{WORKSPACE_NAME}.code-workspace` и директория `.vscode`, в которую будет сохраняться результаты кодогенерации (`.build`) и индекс clangd (`.cache`).

{% note info %}

Открывать проект в VSCode надо через меню `File -> Open Workspace...` или команду `Workspaces: Open Workspace`.

{% endnote %}

### Пример запуска
```
$ ya ide vscode-clangd -P ~/tvmknife-project -j8 --use-arcadia-root --files-visibility=targets-and-deps --setup-tidy --make-args='--target-platform=linux' --make-args='--host-platform=linux' passport/infra/tools/tvmknife
...
Workspace file tvmknife-project.code-workspace is ready
...

$ code ~/tvmknife-project/tvmknife-project.code-workspace
```

### Tasks
Утилита добавляет в воркспейс следующие задачи, доступные через команду `Tasks: Run Task`:

- `<Codegen>` - запуск кодогенерации
- `<Prepare tests>` - подготовить окружение и собрать все тесты проекта
- `<Regenerate workspace>` - перегенерация воркспейса
- `Build ALL (debug)` - сборка всех целей проекта в `debug` профиле
- `Build ALL (release)` - сборка всех целей проекта в `release` профиле
- `Test ALL (<size> tests)` - запуск всех тестов проекта указанного типа через `ya make -t`
- `Build: <binary> (debug)` - сборка конкретного бинарника
- `Prepare test: <test_module> (debug)` - подготовка окружения для отладки тестового модуля
- `Test: <test_module> (debug)` - запуск тестового модуля без отладки.

При изменении набора исходников или зависимостей в проекте, нужно запускать задачу `<Regenerate workspace>`.

### Рекомендуемые расширения
- [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) - основное расширение для автодополнения и навигации по коду
- [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) - расширение IntelliSense для отладки с помощью gdb в VSCode
- [Clang-Tidy](https://marketplace.visualstudio.com/items?itemName=notskm.clang-tidy) - поддержка `clang-tidy`
- [Task Runner](https://marketplace.visualstudio.com/items?itemName=forbeslindesay.forbeslindesay-taskrunner) - удобный интерфейс для отображения и запуска задач (tasks)

### Отладочная конфигурация
`ya ide vscode-clangd` автоматически создаёт для модулей ##PROGRAM## входящих в таргеты отладочные конфигурации вида:

{% cut "Debug Configuration" %}

```json
{
    "launch": {
        "configurations": [
            {
                "name": "arc-local-bin-arc",
                "program": "/Users/viknet/arcadia/arc/local/bin/arc",
                "args": [],
                "environment": [],
                "cwd": "/Users/viknet/arcadia/arc/local/bin",
                "type": "cppdbg",
                "request": "launch",
                "MIMode": "gdb",
                "miDebuggerPath": "/Users/viknet/.ya/tools/v4/1032891985/gdb/bin/gdb",
                "sourceFileMap": {
                    "/-S": "//Users/viknet/arcadia",
                    "/-B": "//Users/viknet/Projects/arc-project/.vscode/.build"
                },
                "setupCommands": [
                    {
                        "text": "-enable-pretty-printing",
                        "description": "Enable pretty-printing for gdb",
                        "ignoreFailures": true
                    }
                ],
                "presentation": {
                    "group": "Run",
                    "order": 0
                },
                "preLaunchTask": "Build 'arc-local-bin-arc' (debug)"
            }
        ],
        "compounds": []
    }
}
```
{% endcut %}

Конфигурации можно запустить с вкладки `Run and Debug`. Перед запуском выполняется соответствующая сборочная задача.

Для запуска можно передать переменные окружения (`environment`) и аргументы командной строки (`args`) отредактировав соответствующую конфигурацию в файле воркспейса. Перенаправление потоков ввода/вывода делается через аргументы:

```
"args": [
    "<", "/file/to/send/to/stdin",
    ">", "/file/to/send/to/stdout"
],
```
Отправка файла в `stdin` поток работает не во всех системах, подробнее про перенаправление можно прочитать в официальной [документации](https://code.visualstudio.com/Docs/editor/debugging#_redirect-inputoutput-tofrom-the-debug-target) VS Code.

Подробнее про процесс дебага (в том числе многопоточных приложений) можно прочитать в официальной [документации](https://code.visualstudio.com/docs/cpp/cpp-debug) VS Code.

### Тесты
При указании флага `--tests` для модулей `UNITTEST` и `GTEST` создадутся отдельные отладочные конфигурации.

Поддержка тестов реализована через механизм `YA_TEST_CONTEXT`, который требует подготовки окружения. Специальная задача `<Prepare tests>` выполняет такую подготовку, а также собирает все тестовые бинарники проекта. При запуске отладки тестов подготовка выполняется автоматически.

Сейчас поддерживаются макросы `DATA` и `DEPENDS`. Рецепты и канонизация в [разработке](https://st.yandex-team.ru/DEVTOOLS-9139).

Запустить отладку тестов можно со страницы `Run and Debug`.

## Golang
`ya ide vscode-go` - утилита, генерирующая VSCode Workspace, совместимый с расширением [Go](https://marketplace.visualstudio.com/items?itemName=golang.go).

Для переданного проекта утилита:

- выполняет кодогенерацию;
- настраивает автодополнение и навигацию по коду;
- ограничивает отслеживание изменений файлов проекта (на Linux отключает полностью, т.к. встроенная поддержка inotify пожирает ресурсы);
- добавляет задачи для кодогенерации, выполнения сборки и перегенерации воркспейса;
- добавляет отладочные конфигурации для приложений и тестов проекта.

### Опции запуска
`ya ide vscode-go [OPTION]... [TARGET]...`

`TARGET ...` - список путей проекта которые нужно включить в workspace и индексацию;

- `-P=PROJECT_OUTPUT` - директория, в которой будет создан воркспейс. По умолчанию - текущая директория;
- `-W=WORKSPACE_NAME, --workspace-name=WORKSPACE_NAME` - название воркспейса. По умолчанию берётся имя директории `PROJECT_OUTPUT`;
- `--no-codegen` - не запускать кодогенерацию при создании воркспейса;
- `--apple-arm-platform` - использовать тулинг под Apple Silicon;
- `--goroot=GOROOT` - указать `GOROOT` для использования стороннего Go-тулчейна;
- `-j=BUILD_THREADS` - количество тредов для индексации, кодогенерации, сборки. По умолчанию равно количеству логических процессоров на машине;
- `--make-args=YA_MAKE_EXTRA` - дополнительные аргументы запуска ya make;
- `-t, --tests` - создать отладочные конфигурации для тестов и подготовить тестовые окружения.

В `PROJECT_OUTPUT` будет создан файл воркспейса `{WORKSPACE_NAME}.code-workspace`.

{% note info %}

Открывать проект в VSCode надо через меню `File -> Open Workspace...` или команду `Workspaces: Open Workspace`.

{% endnote %}

### Пример запуска
```
$ ya ide vscode-go -P ~/ytbench-project -j8 passport/infra/tools/ytbench
...
Workspace file ytbench-project.code-workspace is ready
...

$ code ~/ytbench-project/ytbench-project.code-workspace
```

### Tasks
Утилита добавляет в воркспейс следующие задачи, доступные через команду `Tasks: Run Task`:

- `<Codegen>` - запуск кодогенерации
- `<Prepare tests>` - подготовить окружение и собрать все тесты проекта
- `<Regenerate workspace>` - перегенерация `workspace.code-workspace`
- `Build ALL (debug)` - сборка всех целей проекта в `debug` профиле
- `Build ALL (release)` - сборка всех целей проекта в `release` профиле
- `Test ALL (<size> tests)` - запуск всех тестов проекта указанного типа через `ya make -t`
- `Build: <binary> (debug)` - сборка конкретного бинарника с CGO
- `Prepare test: <test_module> (debug)` - подготовка окружения для отладки тестового модуля
- `Test: <test_module> (debug)` - запуск тестового модуля без отладки.

При изменении набора зависимостей в проекте, нужно запускать задачу `<Regenerate workspace>`

### Рекомендуемые расширения
- [Go](https://marketplace.visualstudio.com/items?itemName=golang.go) - основное расширение для автодополнения и навигации по коду
- [Task Runner](https://marketplace.visualstudio.com/items?itemName=forbeslindesay.forbeslindesay-taskrunner) - удобный интерфейс для отображения и запуска задач (tasks)

### Тесты
При указании флага `--tests` для модулей `GO_TEST` и `GO_TEST_FOR` создадутся отдельные отладочные конфигурации.

Поддержка тестов реализована через механизм `YA_TEST_CONTEXT`, который требует подготовки окружения. Специальная задача `<Prepare tests>` выполняет такую подготовку, а также собирает все тестовые бинарники проекта. При запуске отладки тестов подготовка выполняется автоматически.

Сейчас поддерживаются макросы `DATA` и `DEPENDS`. Рецепты и канонизация в [разработке](https://st.yandex-team.ru/DEVTOOLS-9139).

Запустить отладку тестов можно со страницы `Run and Debug`.

### Особенности
#### CGO
Сборка бинарных зависимостей в Аркадии отличается от принятой во внешнем мире, поэтому автодополнение и навигация по ним не будет работать.

Кодогенерация производится без CGO.

Стандартная отладочная конфигурация `Current Package (without CGO)` и встроенная поддержка тестов используют `CGO_ENABLED=0`.

Остальные сгенерированные отладочные конфигурации для каждого бинарника делают перед запуском `ya make` сборку с CGO.

#### Apple M1
Отладка под Apple Silicon возможна только с ARM64-версией `dlv` и отлаживаемого бинарника.

Флаг `--apple-arm-platform` скачивает ARM64-тулчейн и настраивает сборку бинарников под нужную архитектуру, но перед запуском `ya ide vscode-go` может потребоваться полностью удалить кэш `~/.ya/tools` (при запуске появится предупреждение).

VSCode автоматически устанавливает `gopls`, `dlv` и прочие утилиты в `GOPATH` для той же архитектуры, что и основной компилятор, поэтому их тоже нужно будет удалить, если они уже были установлены. Проверить архитектуру утилит можно командой `file ~/go/bin/dlv`.

## Python
{% note warning %}

С 1 января 2020 года Python 2 объявлен устаревшим и больше не развивается. В связи с этим многие сторонние инструменты, включая Pylance, прекратили его поддержку, поэтому всё нижеследующее относится только к проектам на актуальной версии Python 3.
Поддержка Python 2 пока ещё есть в [JetBrains PyCharm](/ya-make/usage/ya_ide/pycharm)

{% endnote %}

Разработка на Python в аркадии довольно сильно [отличается](https://docs.yandex-team.ru/ya-make/manual/python/) от привычной во внешнем мире инфраструктуры.

`ya ide vscode-py` - утилита, генерирующая VSCode Workspace, совместимый с расширением Microsoft [Python](https://marketplace.visualstudio.com/items?itemName=ms-python.python), использующим Language Server [Pylance](https://marketplace.visualstudio.com/items?itemName=ms-python.vscode-pylance).

Для переданного проекта утилита:

- выполняет кодогенерацию;
- настраивает автодополнение и навигацию по коду;
- ограничивает отслеживание изменений файлов проекта (на Linux отключает полностью, т.к. встроенная поддержка inotify пожирает ресурсы);
- добавляет задачи для кодогенерации, выполнения сборки и перегенерации воркспейса;
- добавляет отладочные конфигурации для приложений и тестов проекта.

### Опции запуска
`ya ide vscode-py [OPTION]... [TARGET]...`

`TARGET ...` - список путей проекта которые нужно включить в workspace и индексацию;

- ` -P=PROJECT_OUTPUT, --project-output=PROJECT_OUTPUT` - директория, в которой будет создан воркспейс. По умолчанию - текущая директория;
- `-W=WORKSPACE_NAME, --workspace-name=WORKSPACE_NAME` - название воркспейса. По умолчанию берётся имя директории `PROJECT_OUTPUT`.
- `--no-codegen` - не запускать кодогенерацию при создании воркспейса;
- `--use-arcadia-root` - использовать в качестве workspaceFolder корень аркадии (иначе будет использованы директории проекта);
- `--files-visibility=FILES_VISIBILITY` - в сочетании с `--use-arcadia-root` позволяет ограничить список директорий для отображения и поиска. `FILES_VISIBILITY` принимает значения:
    - `targets-and-deps` - отображать директории проекта и зависимостей (default);
    - `targets` - только директории проекта;
    - `all` - не ограничивать видимость.
- `-j=BUILD_THREADS` - количество тредов для индексации, кодогенерации, сборки. По умолчанию равно количеству логических процессоров на машине;
- `--make-args=YA_MAKE_EXTRA` - дополнительные аргументы запуска ya make;
- `-t, --tests` - создать отладочные конфигурации для тестов и подготовить тестовые окружения.


В `PROJECT_OUTPUT` будут созданы:

- файл воркспейса `{WORKSPACE_NAME}.code-workspace`;
- директория `venv` с интерпретатором, созданным с помощью [ya ide venv](/ya-make/usage/ya_ide/venv), в который собраны все зависимости проекта;
- директория `.links`, в которую подкладываются симлинки на зависимости с NAMESPACE в понятном Pylance виде;
- директория `test_wrappers`, содержащая псевдо-интерпретаторы для запуска отладки тестов.

{% note info %}

Открывать проект в VSCode надо через меню `File -> Open Workspace...` или команду `Workspaces: Open Workspace`.

{% endnote %}

После открытия проекта может потребоваться установить интерпретатор по-умолчанию - нужно выбрать venv из папки воркспейса.

### Пример запуска
```
$ ya ide vscode-py --no-codegen -P ~/Projects/ya-project --tests devtools/ya
...
Workspace file /Users/viknet/Projects/ya-project/ya-project.code-workspace is ready
... 
$ code ~/Projects/ya-project/ya-project.code-workspace
```

### Tasks
Утилита добавляет в воркспейс следующие задачи, доступные через команду `Tasks: Run Task`:

- `<Codegen>` - запуск кодогенерации
- `<Prepare tests>` - подготовить окружение и собрать все тесты проекта
- `<Rebuild venv>` - 
- `<Regenerate workspace>` - перегенерация `workspace.code-workspace`
- `Build ALL (debug)` - сборка всех целей проекта в `debug` профиле
- `Build ALL (release)` - сборка всех целей проекта в `release` профиле
- `Test ALL (<size> tests)` - запуск всех тестов проекта указанного типа через `ya make -t`
- `Build: <binary> (debug)` - сборка конкретного бинарника с CGO
- `Prepare test: <test_module> (debug)` - подготовка окружения для отладки тестового модуля
- `Test: <test_module> (debug)` - запуск тестового модуля без отладки.

При изменении набора зависимостей в проекте, нужно запускать задачу `<Regenerate workspace>`

### Рекомендуемые расширения
- [Python](https://marketplace.visualstudio.com/items?itemName=ms-python.python) - основное расширение для автодополнения и навигации по коду
- [Pylance](https://marketplace.visualstudio.com/items?itemName=ms-python.vscode-pylance) - LSP от Microsoft на основе Pyright
- [Task Runner](https://marketplace.visualstudio.com/items?itemName=forbeslindesay.forbeslindesay-taskrunner) - удобный интерфейс для отображения и запуска задач (tasks)

### Тесты
При указании флага `--tests` для модулей `PY3TEST` и `PY23_TEST` создадутся отдельные отладочные конфигурации.

Поддержка тестов реализована через механизм `YA_TEST_CONTEXT`, который требует подготовки окружения. Специальная задача `<Prepare tests>` выполняет такую подготовку, а также собирает все тестовые бинарники проекта. При запуске отладки тестов подготовка выполняется автоматически.

Сейчас поддерживаются макросы `DATA` и `DEPENDS`. Рецепты и канонизация в [разработке](https://st.yandex-team.ru/DEVTOOLS-9139).

Запустить отладку тестов можно со страницы `Run and Debug`.

### Особенности и проблемы
-  `ya ide venv` не поддерживается на платформе Windows, соответственно, `ya ide vscode-py` работать тоже не будет;
-  некоторые модули могут не индексироваться - это связано с несколькими причинами:
	- нестандартная сборка Cython-модулей в Аркадии - Pylance их просто не находит;
	- несколько модулей с одним NAMESPACE - Pylance возьмёт в индекс только один из них (как пример - `yt.packages`);
	- ограничения статического анализатора, не позволяющие находить динамически создаваемые модули и объекты (как пример - `six.moves`);
- встроенная в Python-плагин поддержка pytest ломается на любых тестах, использующих аркадийную машинерию, что приводит к полному отказу от обнаружения остальных тестов;
- запуск PY3_PROGRAM с точкой входа PY_MAIN, указывающей на конкретную функцию, на данный момент не работает.


## VS Code Remote
Для удалённой разработки существует удобное расширение [Remote - SSH](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh).

Чтобы подготовить VS Code для удалённой работы нужно:

- установить расширение `Remote - SSH` на вкладке Extensions
- выполнить команду `Remote-SSH: Connect to Host`, передав ей hostname удалённой машины

Если подключение не удалось, возможно у вас не настроен [SSH-Agent](https://wiki.yandex-team.ru/security/ssh/#instrukciiponastrojjkessh-klientov).

В процессе подключения расширение самостоятельно установит и запустит VSCode Server.

После подключения VS Code работает как тонкий клиент, все действия внутри IDE выполняются на удалённой машине.

Дальнейшая настройка аркадийного проекта аналогична локальной, для выполнения консольных команд можно использовать встроенный терминал:

- на вкладке Extensions установить рекомендуемые расширения
- [подготовить Arc-репозиторий](/devtools/intro/quick-start-guide)
- запустить `ya ide vscode-(clangd/go/py) -P </path/to/workspace/directory> -j8 </path/to/arcadia/project>`
- открыть сгенерированный файл воркспейса командой `Open Workspace`.

{% note info %}

При запуске `ya ide vscode-(clangd/go/py)` в SSH-сессии в выводе команды появится ссылка вида `vscode://vscode-remote/ssh-remote+{host}{workspace_path}`, на которую достаточно кликнуть, чтобы открыть удалённый проект в локальном VS Code через Remote-SSH плагин.

Необходимые условия:

- терминал поддерживает открытие ссылок
- VS Code установлен на локальной машине
- ssh-ключ добавлен в ssh-агент (`ssh-add -K ~/.ssh/id_rsa`).

{% endnote %}
