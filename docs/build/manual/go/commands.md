# ya tool yo

Редактированея ya.make-ов и управление директорией vendor в аркадии.

`ya tool yo`

## Работа с vendor

### Шаг 1: Запустить `yo get`

```bash
$ ya tool yo get go.uber.org/zap # get last version
$ ya tool yo get go.uber.org/zap@v1.14.1 # get v1.14.1
```

`yo get` принимает одно имя модуля, в формате `go get`.

### Шаг 2: Обновить `vendor.policy`

Файл `build/rules/go/vendor.policy` задаёт список разрешённых пакетов. В `vendor/`
хранится исходный код тех пакетов, которые нужны для сборки разрешённых.

Например, в случае с `aws-go`, мы добавляем в `vendor/` не все клиенты, а только `s3`
и `sqs`.

```
ALLOW .* -> vendor/github.com/aws/aws-sdk-go/aws
ALLOW .* -> vendor/github.com/aws/aws-sdk-go/service/s3
ALLOW .* -> vendor/github.com/aws/aws-sdk-go/service/sqs
```

### Шаг 3: Запустить `yo vendor`

```
ya tool yo vendor
```

Команда `yo vendor` анализирует изменения в `vendor.policy` и `go.mod`, и обновляет
файлы внутри `vendor/`

### Шаг 4: Проверить сборку и тесты нового кода

```
$ ya make -r -j0 vendor/my/new/package # проверить конфигурацию
$ ya make -r vendor/my/new/package     # проверить сборку
$ ya make -r -t vendor/my/new/package  # проверить тесты
```

## Автоматическая генерация ya.make

Утилита умеет автоматически редактировать список исходников внутри
ya.make файлов. Этой функция доступна через команду `yo fix`. Ей можно
воспользоваться отдельно, в произвольном проекте аркадии.

`yo fix` занимается именно редактированием, а не генерацией. `ya.make`
файлы могут содержать макросы, которые невозможно сгенерировать из исходного
кода. `yo fix` старается не трогать части `ya.make`, которые ему не
понятны.

## Как обновить версию yo внутри ya tool

1. Склонировать задачу https://sandbox.yandex-team.ru/task/430421260/view
2. В поле `Svn url for arcadia` удалить ревизию (число после @). Если этого не сделать,
   соберётся старая версия.
3. Дождаться завершения задачи. В списке child tasks должны появиться задачи сборки
   под linux и darwin.
4. ID двух дочерних задач нужно вписать в файл `build/ya.conf.json`. Порядок важен.
5. Локальным запуском `ya tool yo version` проверить, что платформы linux и darwin не
   перепутались.
6. Закоммитить изменения.
