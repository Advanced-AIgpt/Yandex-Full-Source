# Разработка в Аркадии с JetBrains PyCharm
[JetBrains PyCharm](https://www.jetbrains.com/pycharm) является одной из самых популярных IDE для разработки на Python в Яндексе.
Для работы с PyCharm существует [плагин](https://docs.yandex-team.ru/devtools/src/ide#jb-plugin) и хендлер **ya ide pycharm**, упрощающий настройку запуска и отладки тестов из PyCharm.

## IntelliJ Arcadia plugin
Помимо интеграций с Arc VCS и Arcanum, [IntelliJ Arcadia Plugin](https://docs.yandex-team.ru/devtools/src/ide#jb-plugin) так же добавляет интеграцию PyCharm с аркадийной сборкой.
1. Разрешение импортов в Python-файлах и их автодополнение по описанию сборки в файлах ya.make.
2. Навигация по ya.make файлам.
3. Автодополнение макросов в ya.make.

Плагин рекомендуется к установке для всех пользователей PyCharm в Аркадии, даже если проект написан на Python2.

[Инструкция](https://docs.yandex-team.ru/devtools/intro/quick-start-guide#jb-plugin-setup) по установке.

## Поддерживаемые флаги
* `--only-generate-wrapper`: не генерировать проект, только обертки для найденных целей.
* `--wrapper-name=WRAPPER_NAME`: задать альтернативное имя обертки. Необходимо для ручного добавления интерпретатора в PyCharm.
* `--ide-version=IDE_VERSION`: выбрать специфичную IDE для обновления списка доступных в IDE интерпретаторов.
* `--list-ide`: показать доступные для обновления списка IDE.

Так же поддерживаются флаги [--yt-store](../ya_make/yt_store.md) и [--dist](../ya_make/dist.md), как в [ya make](../ya_make/index.md).

## Ограничения и недостатки
1. Требуется Python3 проект. Если у вас используются PY23 модули - это тоже нормально, ya ide pycharm их подхватит.
2. Операционная система: Linux или MacOS. Поддержка Windows для `ya ide pycharm` отсутствует, подсветка импортов и автодополнение, предоставляемое плагином, должны работать нормально.
3. Не заработают тестовые рецепты, подкачивание ресурсов из Sandbox и DATA.
4. При использовании SVN необходимо докачать зависимости проекта селективным чекаутом при помощи `ya make -j0 --force-build-depends --checkout`.
5. Надо пересобирать бинари при добавлении новой зависимости или обновлении плюсового кода.

## Как воспользоваться?
1. Убедиться, что вы хотя бы раз запускали PyCharm и настроили там хотя бы один Python SDK руками.
2. Закрыть PyCharm.
3. Перейти в корневую директорию нужного вам проекта. К примеру, в `travel/avia/subscriptions`.
4. Запустить команду `ya ide pycharm` и дождаться окончания сборки.
5. Открыть в PyCharm директорию проекта. К примеру, при помощи `pycharm .`, если у вас сгенерирован соответствующий shell скрипт.
6. Убедитесь, что в качестве интерепретатора на весь проект у вас выставлен системный Python.
7. А собственно все. У вас есть многомодульный проект и запуск тестов должен работать как если бы вы настраивали руками.


## Удаленная разработка и отладка
На данный момент `ya ide pycharm` позволяет несколько упростить создание оберток для запуска тестов.
Для этого нам надо:
1. Зайти на виртуалку в директорию проекта: `cd ~/arc/arcadia/travel/avia/subscriptions`.
2. Сгенерировать только обертки, задав для них имя `python`: `ya ide pycharm --only-generate-wrapper --wrapper-name=python`.
3. Добавить выбранную обертку как удаленный интерпретатор [средствами PyCharm](https://www.jetbrains.com/help/pycharm/configuring-remote-interpreters-via-ssh.html#ssh).

## Python 2
Если по историческим причинам вы работаете с проектом на Python 2 и, соответственно, не можете воспользовать `ya ide pycharm`, аналогичные операции можно проделать самостоятельно.
Описано это в [посте Ильи Дьячкова](https://i-dyachkov.at.yandex-team.ru/1). 

## Дополнительные ссылки
- [Базовая идея оберток](https://i-dyachkov.at.yandex-team.ru/1)
- [PyCharm Helper: предтеча ya ide pycharm](https://clubs.at.yandex-team.ru/python/3386)
- [Анонс ya ide pycharm](https://clubs.at.yandex-team.ru/python/3392)

