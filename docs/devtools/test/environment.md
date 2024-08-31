# Подготовка окружения

В данном разделе описаны рекомендуемые подходы по подготовке тестового окружения: запуску тестовых копий сервисов, от которых зависит ваше приложение, подготовке данных и так далее.

## Переменные окружения { #variable }

Тестам можно выставлять произвольные переменные окружения при помощи макроса `ENV`:

```yamake
OWNER(g:my-group)

JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

ENV(LANG=ru_RU.UTF8 TZ=Europe/Moscow) # Переменные окружения

END()
```

## Рецепты { #recipes }

Часто различным тестам нужно сначала подготовить для себя окружение: запустить дополнительные сервисы или их заглушки, подготовить данные и так далее. Обычно, код, который этим занимается, существует как часть теста и работает в том же процессе: пишется на том же языке программирования с использованием текущего тестового фреймворка. Такой подход сильно усложняет использование этого кода в других тестах, например, процедуру подготовки тестового стенда, написанную на Python, практически нельзя переиспользовать в тестах на C++. Эту проблему решают **Рецепты (recipe)**.

Рецепт позволяет перенести запуск тестового окружения на отдельный уровень. Он может:

* Настроить и запустить необходимые программы;
* Сообщить тесту всю необходимую информацию о подготовленном окружениии;
* Корректно остановить стенд после прогона теста.

Рецепт представляет собой программу, которая запускается в двух режимах:

1. **Start**
    * Запускается перед тестом;
    * Имеет с тестом общую рабочую директорию;
    * Может задать переменные окружения для теста;
    * Имеет доступ ко всем DEPENDS / DATA, то есть может запускать другие программы так же, как это делает тест;
    * Завершается перед тестом, оставляя после себя запущенный стенд.
2. **Stop**
    * Запускается после теста;
    * Имеет с тестом общую рабочую директорию, из которой может извлечь информацию о ранее запущенном стенде и погасить его.

Для того, чтобы подключить рецепт к тесту, нужно воспользоваться макросом `USE_RECIPE`
в **ya.make**:

```yamake
OWNER(g:my-group)

JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

USE_RECIPE(my-project/recipes/test-api) # Используем рецепт, описанный в указанном каталоге

END()
```

Некоторые наиболее часто используемые рецепты можно найти в каталоге [library/recipes](https://a.yandex-team.ru/arc/trunk/arcadia/library/recipes). При этом, поскольку рецепт может иметь свои зависимости, распространенной практикой является использование ***.inc** файлов, в которых объявлены все макросы, необходимые для подключения конкретного рецепта. Такой *.inc файл можно добавить в свой **ya.make** при помощи инструкции `INCLUDE`:

```yamake
OWNER(g:my-group)

JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

# Поднимаем окружение при помощи Docker Compose.
# Макрос USE_RECIPE находится внутри *.inc файла
# и происходит текстовая подстановка всех макросов из recipe.inc в на место INCLUDE.
INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose/recipe.inc)

END()
```

## Как написать рецепт { #create-recipe }

{% note info %}

В настоящий момент рецепты можно писать на [Python](https://python.org/) и [Go](https://golang.org/).

{% endnote %}

Рецепт — это обычный исполняемый файл, содержащий методы `start` и `stop`. Минимальный [пример](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/recipe/recipe) на Python:

```python
from library.python.testing.recipe import declare_recipe

def start(argv):
    # Setup code
    print 'Starting...'

def stop(argv):
    # Teardown code
    print('Stopping...')

if __name__ == "__main__":
    declare_recipe(start, stop)
```

Аналогичный [пример](https://a.yandex-team.ru/arc/trunk/arcadia/yt/go/ytrecipe/cmd/ytrecipe) на Go:

```go
package main

import (
    "a.yandex-team.ru/library/go/test/recipe"
    "a.yandex-team.ru/library/go/test/yatest"
    "fmt"
)

type myRecipe struct{}

func (y *myRecipe) Start() error {
    fmt.Println("Starting...")
    return nil
}

func (y *myRecipe) Stop() error {
    fmt.Println("Stopping...")
    return nil
}

func main() {
    recipe.Run(myRecipe{})
}
```

## Использование Docker Compose { #docker-compose }

{% note info %}

Полный пример использования Docker Compose можно посмотреть [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/library/recipes/docker_compose/example).

{% endnote %}

Примером случая, когда вам могут потребоваться рецепты, является использование [Docker Compose](https://docs.docker.com/compose/) для поднятия тестового окружения.

1. Создайте файл `docker-compose.yml` и каталоге с тестами.
2. Файл **ya.make** будет выглядеть примерно так:

```yamake
OWNER(g:my-group)

JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

    INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose/recipe.inc)
SIZE(LARGE) # Docker можно запускать только в Sandbox, т.е. в LARGE тестах

TAG(
    ya:external
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

REQUIREMENTS(
    container:3342470530 # В этом контейнере уже есть Docker и выполнена аутентификация в registry.yandex.net от имени пользователя arcadia-devtools
    cpu:all dns:dns64
)

END()
```

Стандартные контейнеры с Docker из Sandbox, готовые к использованию:

1. 3342470530 (ubuntu 18.04-bionic, python 3.6.9, docker 20.10.17), рекомендуется к использованию
2. 823446926 (ubuntu 16.04-xenial, python 3.5.2, docker 18.09.1)
