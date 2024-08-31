# Dialogovo – место, где живут диалоги

Dialogovo – платформа работы с внешними навыками. Компонент отвечает за выполнение запросов в навыки, сбор статистики и сервисы, предоставляемые внешним навыкам.

## IDEA
Для создания и настройки проекта нужно выполнить `./gen_project.sh`. Он создаст проект в `~/IdeaProjects/dialogovo`.
<br>
Можно и вручную, если хочется настроить project-root:
```bash
ya ide idea --project-root="/Users/$USER/IdeaProjects/dialogovo" --local --directory-based --iml-in-project-root --generate-junit-run-configurations
```

Далее стоит импортировать настройки code style, если этого не было сделано ранее. Ссылка на настройки фигурирует в логе `ya ide idea`, примерно вот так:
```
Info: Successfully generated idea project: /Users/$USER/IdeaProjects/dialogovo
Info: Devtools IntelliJ plugin (Latest stable IDEA is required): https://a.yandex-team.ru/arc/trunk/arcadia/devtools/intellij/README.md
Info: Codestyle config: /Users/$USER/.ya/tools/v4/441758406/intellij-codestyle.jar. You can import this file with "File->Import settings" command. After this choose "yandex-arcadia" in code style settings (Preferences -> Editor -> Code Style).
```

### Для компиляции проекта в IDEA

Проверяем, что у нас установлена последняя версия плагина для Kotlin: `Preferences -> Languages & Frameworks -> Kotlin`, нажимаем `Check again`. Устанавливаем обновление, если нужно.
<br>
Далее выставляем последнюю версию в Kotlin Compiler: `Preferences -> Build, Execution, Deployment -> Compiler | Kotlin Compiler`. Ставим самые новые `Language version`, `API version` и `Taget JVM version` из доступных.

Используемые в проекте флаги (скорее всего уже стоят, ничего делать не нужно):
```shell
-version -Xjvm-default=all -java-parameters -Xopt-in=kotlin.RequiresOptIn
```

Чтобы избавиться от "покраснения", нужно установить Lombok plugin (скорее всего уже стоит).

Чтобы тесты запускались как надо: добавить в VM options template's JUnit
 ```
-Djava.library.path=/Users/$USER/IdeaProjects/dialogovo/dlls -Djna.library.path=/Users/$USER/IdeaProjects/dialogovo/dlls
```

> Важно:
> После обновления исходников может потребоваться пересборка проекта. Запускаем `./gen_project.sh`(помни, что он собирает в `~/IdeaProjects/dialogovo`), закрывает проект и открываем заново.
> Если это не решило проблем, можно удалить из `/Users/$USER/IdeaProjects/dialogovo` всё содержимое кроме папки `.idea`, и после этого запустить `ya ide idea`.

## Запуск на виртуалке

Запускать VM нужно тут https://qyp.yandex-team.ru. В качестве Network Macros нужно использовать **\_ALICEDEVNETS\_**,
потому что в нём уже настроены все необходимые доступы. Если нет прав на использования макроса, то нужно добавить себя
сюда [ALICEDEVNETS_ACL](https://racktables.yandex-team.ru/index.php?page=services&tab=vperms#fws-ALICEDEVNETS)

``$ARCADIA`` - путь до Аркадии. Проще всего добавить строчку вида `export ARCADIA=<path_to_arc>` в `~/.bashrc`

> Предварительно к VM нужно подключить рабочее дерево **arc**. Подробнее
>тут [Arcadia Starter Guide](https://wiki.yandex-team.ru/arcadia/Arcadia-Starter-Guide/)

### Запуск tvmtool

Подробнее про tvmtool: [Tvm daemon](https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/)

1\. Устанавливаем tvmtool из репозитория.

В название репы вписываем нужную версию ubuntu

```bash
sudo add-apt-repository "deb http://dist.yandex.ru/yandex-bionic stable/all/"
sudo apt update
sudo apt install yandex-passport-tvmtool
```

2\. Копируем и правим конфиг

```bash
mkdir ~/tvmtool
cd ~/tvmtool
cp $ARCADIA/alice/paskills/dialogovo/misc/tvm.priemka.json ./tvmtool.conf
```

3\. Создаём файл с локальным tvm-токеном:
```shell
xxd -l 16 -p /dev/urandom > ./token.txt
```

4\. Создаем файл ``~/tvmtool/dialogovo.secret`` и вставляем туда _секрет приложения_ (взять у коллег).
Скорее всего он тут: https://yav.yandex-team.ru/secret/sec-01dkh12tz28a8je6xt74j8870h/
Копируй из tvm_secret

5\. Открываем файл ``~/tvmtool/start_tvmtool.sh`` и заменяем содержимое:

```bash
#!/bin/bash
export QLOUD_TVM_TOKEN=`cat ./token.txt`
export TVM_SECRET=`cat ./dialogovo.secret`
tvmtool -v -e --port 8001 -c tvmtool.conf
```

и помечаем его выполняемым
```shell
chmod +x start_tvmtool.sh
```

6\. Запускаем:

```bash
./start_tvmtool.sh
```

### Сборка и раскладка dialogovo

```bash
cd $ARCADIA/alice/paskills/dialogovo
ya make
```
Копируем сертификат для postgres:
```bash
mkdir ~/.postgresql
cp ./docker-base-image/root.crt ~/.postgresql/
```

### Запуск

Перед запуском не забудь настроить SSH ключи на VM (https://wiki.yandex-team.ru/plus/backend/expert-support/generacija-ssh/)
<br>
Для запуска выполни:
```bash
./dialogovo.sh --port 8000 --debug --make
```

Ключ `--debug` позволяет подключиться отладчиком по 5005 порту. Для этого понадобится пробросить порт через
ssh-соединение командой `ssh dev-vm -L 5005:localhost:5005`
<br>
Ключ `--make` указывает на необходимость пересобрать проект через ya make перед запуском.

> Перестал запускаться проект? (например, по NoClassDefFound) <br>
> Может помочь `arc clean -d -x` (на vm тоже стоит запустить), это почистит старые файлы, которые могут помешать верной сборке, но нужно помнить, что все неотслеживаемые арком файлы пропадут (можно сделать `arc add <файл>` на все, что нужно)

### Совет для быстрой разработки на VM
Запускаем синхронизацию кода через `arc sync`(wiki: https://docs.yandex-team.ru/arc/ref/commands#sync). На локальной машине выполните, например:
```bash
arc sync begin --interval 5 --include-untracked
```
Скопируйте команду, которая будет выведена (пример: `arc sync follow app/wave-stash/user/wave-stash/abc`) и запустите на VM.
Далее просто выполняйте на VM `./dialogovo.sh --port 8000 --debug --make` каждый раз, когда нужно запустить новый код.

## Отладка (iOS)

Приложение для тестирования
> Для авторизации в нужно использовать личную почту привязанную на стаффе

https://beta.m.soft.yandex.ru/description?app=yandex&platform_shortcut=iphoneos&branch=master

###Подменить компонент dialogovo на свой экземпляр

Панель отладки → Assistant → Settings → Vins url - устанавливаем значение ниже, подставив свой хост
> Хост можно взять из настроек виртуальной машины в поле FQDN
```
http://megamind-rc.alice.yandex.net/speechkit/app/pa/?x-srcrwr=Dialogovo:http://<host>:8021/megamind/
```

Удобно использовать QR-коды:

```bash
sudo apt install qrencode
qrencode -o code.png <строка>
```

###Установка флагов экспериментов:

Панель отладки → Assistant → Settings → Experiment flags

Пишем флаги через запятую без пробелов, в формате флаг=значение. После этого нужно перезапустить приложение.

## Сборка и пуш docker образа

Чтобы собрать докер нужно вызвать
```./package.sh```

Можно сразу собрать и запушить образ
```
./package.sh --docker-push
```

С указанием версии образа, отличной от формата registry.yandex.net/{repository}/{package name}:{package version}
```
./package.sh --docker-push  --custom-version={version name}
```

В `pkg.json` используется список файлов исключений, чтобы разделить в docker слои из внешних библиотек
и аркадийных, которые часто меняются (внутри содержат timestamp билда и ревизию).

Получить этот список можно выполнив в корне dialogovo:
```
ya make && ls -1 dialogovo | grep -v "\-[0-9]" | sed 's/\.dylib/.so/g'
```

В первом списке исключений в `pkg.json` файл `dialogovo.jar` учитывается, во втором, где копируются только аркадийные артефакты dialogovo.jar быть не должно, он копируется отдельным шагом.

## Старые инструкции

```docker build --target release -t "registry.yandex.net/paskills/dialogovo:r`svn info -r HEAD |grep Revision: |cut -c11-`" .```

```docker build -f LocalDockerfile --target release -t "registry.yandex.net/paskills/dialogovo:r`svn info -r HEAD |grep Revision: |cut -c11-`" .```

Ребейз:
```
arc reset --soft $(arc merge-base HEAD trunk) && arc add . && arc commit -am "squash" && arc rebase trunk
```

Выгрузка дампа для тестовой БД:
```
pg_dump -s --no-owner -x -d paskillsdb_priemka -U paskills -p 6432 -h c-5be6ef44-409b-4f4b-bff7-336545400887.ro.db.yandex.net > create.sql
```
Послы нужно по диффу закомментировать лишние объекты, а также убрать удалить `COLLATE "en_US"` из скрипта
