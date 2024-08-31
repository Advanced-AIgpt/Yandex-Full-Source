# Пакетирование

{% note info %}

Подробно создание пакетов описано в документации по системе сборки:

* [Использование команды `ya package`](https://docs.yandex-team.ru/ya-make/usage/ya_package)
* [Формат JSON-описания пакета](https://docs.yandex-team.ru/ya-make/usage/ya_package/json)

{% endnote %}

Когда вы [собрали](build/index.md) и [протестировали](test/intro.md) свой проект, его нужно выложить на удаленный сервер. Для этого обычно все получившиеся исполняемые файлы и зависимости упаковываются в архив специального формата, называемый **пакетом** (**package**). Процесс создания пакета называется **пакетированием**.

## Сборка пакета { #build-package }

Для сборки пакетов используется отдельная команда `ya package`. Пример вызова команды:

```bash
$ ya package --debian path/to/package.json # Собрать пакет в виде Debian архива
```

Описание содержимого пакета хранится в специальном файле `package.json`, который может располагаться в любом каталоге единого репозитория. Подробное описание этого файла можно найти [здесь](https://docs.yandex-team.ru/ya-make/usage/ya_package/json). В файле описывается:

* Метаданные пакета: название, версия и так далее;
* Конфигурация для сборки артефактов, складываемых в пакет;
* Источники и размещение файлов в пакете;
* Дополнительные действия, которые необходимо выполнить при сборке пакета (постобработка).

Рассмотрим пример сборки [tar](https://en.wikipedia.org/wiki/Tar_(computing)) архива:

```bash
$ ya package --tar devtools/dummy_arcadia/package/hello_world.json
...
$ tar -tvzf yandex-package-hello-world.85c7e374108166bfc1b2a47ca888830965a07708.tar.gz
drwxrwxr-x 0/0               0 2021-03-11 06:58 some_package_dir/
-rwxrwxr-x 0/0           19488 2021-03-11 06:58 some_package_dir/hello_world
```

Содержимое файла `hello_world.json` выглядит так:

{% code '/devtools/dummy_arcadia/package/hello_world.json' lang='json' %}

В этом файле описано, что:

* Именем пакета будет `yandex-package-hello-world`;
* Версией пакета будет текущая ревизия репозитория, которую система сборки определит сама;
* Для пакета надо собрать программу [devtools/dummy_arcadia/hello_world](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/dummy_arcadia/hello_world/ya.make);
* Собранную программу нужно сложить в `some_package_dir`.

## Поддерживаемые типы пакетов { #supported-packages }

В настоящий момент поддерживается сборка следующих типов пакетов:

Тип пакета | Пример команды | Документация
:--- | :--- | :---
[aar](https://developer.android.com/studio/projects/android-library.html#aar-contents) пакет | `ya package --aar package.json` | [Ссылка](https://docs.yandex-team.ru/ya-make/usage/ya_package/aar)
[deb](https://en.wikipedia.org/wiki/Deb_(file_format)) пакет | `ya package --debian package.json` | [Ссылка](https://docs.yandex-team.ru/ya-make/usage/ya_package/deb)
[npm](https://en.wikipedia.org/wiki/Npm_(software)) пакет | `ya package --npm package.json` | [Ссылка](https://docs.yandex-team.ru/ya-make/usage/ya_package/npm)
[rpm](https://en.wikipedia.org/wiki/RPM_Package_Manager) пакет | `ya package --rpm package.json` | [Ссылка](https://docs.yandex-team.ru/ya-make/usage/ya_package/rpm)
[tar](https://en.wikipedia.org/wiki/Tar_(computing)) архив | `ya package --tar package.json` | [Ссылка](https://docs.yandex-team.ru/ya-make/usage/ya_package/tar)
Python [wheel](https://en.wikipedia.org/wiki/Setuptools#Package_format) пакет | `ya package --wheel package.json` | [Ссылка](https://docs.yandex-team.ru/ya-make/usage/ya_package/wheel)

## Сборка пакетов в Sandbox { #sandbox }

Для сборки пакетов для production-окружения крайне не рекомендуется запускать `ya package` локально. В пакет могут попасть локальные изменения, а также системное окружение может влиять на сам процесс сборки.  Чтобы исключить перечисленные проблемы, необходимо собирать пакеты через задачу [YA_PACKAGE_2](https://sandbox.yandex-team.ru/tasks?type=YA_PACKAGE_2), которая своими параметрами повторяет ключи команды `ya package`, а также имеет все необходимые для сборки пакетов зависимости.

Больше информации о задаче можно найти [здесь](https://docs.yandex-team.ru/ya-make/usage/sandbox/ya_package).
