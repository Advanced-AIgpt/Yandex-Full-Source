# Быстрый старт

Вы можете быстро развернуть готовый сценарий на своем сервере и протестировать его. Этот пример поможет разобраться в том, как работают сценарии и их взаимодействие с Мегамайндом.

По умолчанию реализовывать собственный сценарий рекомендуется с
помощью [Голливуда](hollywood/index.md). Но если вам этот способ по каким-то
причинам не подходит — можно написать и полностью свой сервер для
обработки запросов от Мегамайнда. К отдельным сервисам применяются
такие же требования как и ко всей Алисе, про эти требования
можно подробнее почитать на [вики](https://wiki.yandex-team.ru/alice/megamind/protocolscenarios/releases).

Примеры реализации различных сценариев можно смотреть [тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios).

Сценарии, которые переведены на новый фреймворк:
* ["Bluetooth"](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/bluetooth).
    Самый простой сценарий. Три простые сцены на каждый возможный вариант релевантного ответа. Основная логика в сценария в Dispatch.
* ["Загадай случайное число"](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/random_number).
    Пример с двумя сценами со сложной логикой внутри них. Есть примеры работы со слотами.
* ["Время и дата"](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/get_date).
    Сложный сценарий. Используются элллипсы, сохранение состояния сценария для следующих запросов пользователя, работа со слотами, сложная работа с рендерингом ответа.
* ["Видео"](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/video).
    Сценарий со сложным графом. Есть примеры походов в источники. *Находится в состоянии переезда...*

## Создание заготовки нового сценария в Голливуде

Запускаем скрипт в [alice/hollywood/hw](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/hw)
```
ya make -r && ./hw create -n fooBar --abc megamind
```

```bash
usage: hw [-h] {create,cr,run} ...

optional arguments:
  -h, --help       show this help message and exit

subcommands:
  {create,cr,run}
    create (cr)    create new HW scenario
    run            run Apphost, MM, HW with local diff

subcommands usage:
    usage: hw create [-h] -n NAME --abc ABC [--shard SHARD] [-f]
    usage: hw run [-h]
```

```bash
usage: hw create [-h] -n NAME --abc ABC [--shard SHARD] [-f]

optional arguments:
  -h, --help            show this help message and exit
  -n NAME, --name NAME  Scenario name (default: None)
  --abc ABC             Abc services (default: None)
  --shard SHARD         Hollywood shards (default: ['all', 'common'])
  -f, --force           force to rebuild megamind configs (default: False)
```

{% note warning %}

Сценарий создается с использованием нового Hollywood фреймворка.
https://docs.yandex-team.ru/alice-scenarios/hollywood/#hollywood-framework

{% endnote %}

Что получим:
* все сценарные конфиги в [alice/megamind/configs](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs)
* необходимые графы в [apphost/conf/verticals/ALICE](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE)
* сценарий в [alice/hollywood/library/scenarios](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios) (nlg, it2, unit test включены)
* сценарий добавляется в `RECURSE_FOR_TESTS` [alice/hollywood/library/scenarios/ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/ya.make)
* сценарий добавляется в указанные `--shard` [alice/hollywood/shards](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/shards)
* добавляется константа в [alice/library/analytics/common/product_scenarios.h](https://a.yandex-team.ru/arc/trunk/arcadia/alice/library/analytics/common/product_scenarios.h)

Сценарий `alice/hollywood/library/scenarios/{NAME}/` должен компилироваться, а юнит тесты проходить.

В сценарии прописывается тестовый фрейм `personal_assistant.scenarios.{NAME}_hello_world`.

{% cut "Пример изменений после запуска скрипта" %}
```
$ arc st
On branch yh_util
Your branch is up-to-date with 'arcadia/users/mihajlova/yh_util'.
Changes not staged for commit:
  (use "arc add/rm <file>..." to update what will be committed)
  (use "arc checkout <file>..." to discard changes in working directory)

     modified:    ../library/scenarios/ya.make
     modified:    ../shards/all/dev/hollywood.pb.txt
     modified:    ../shards/all/prod/hollywood.pb.txt
     modified:    ../shards/all/scenarios/ya.make
     modified:    ../shards/common/prod/hollywood.pb.txt
     modified:    ../shards/common/scenarios/ya.make
     modified:    ../../library/analytics/common/product_scenarios.h
     modified:    ../../../apphost/conf/verticals/ALICE/_custom_alerts.json
     modified:    ../../../apphost/conf/verticals/ALICE/megamind_scenarios_run_stage.json

Untracked files:
  (use "arc add <file>..." to include in what will be committed)

    ../library/scenarios/foo_bar/
    ../../megamind/configs/dev/scenarios/FooBar.pb.txt
    ../../megamind/configs/hamster/scenarios/FooBar.pb.txt
    ../../megamind/configs/production/scenarios/FooBar.pb.txt
    ../../megamind/configs/rc/scenarios/FooBar.pb.txt
    ../../../apphost/conf/verticals/ALICE/foo_bar_run.json

no changes added to commit (use "arc add" and/or "arc commit -a")
```
{% endcut %}

{% cut "Пример компиляции с тестами" %}
```
alice/hollywood/library/scenarios/foo_bar$ ym -tP
Warn[-WUserWarn]: in $S/build/rules/py2_deprecation/ya.make: WARNING You are using deprecated Python2-only code. Please consider rewriting to Python 3. To list all such errors use `ya make -DFAIL_PY2`.
Warn[-WUserWarn]: in $S/alice/hollywood/library/scenarios/foo_bar/it2/ya.make: Если вы перезапускаете тест, не меняя C++ код, то используйте флаг -DNOCOMPILE для более быстрого запуска
Number of suites skipped by size: 1

alice/hollywood/library/scenarios/foo_bar/it2 <flake8.py3>
------ sole chunk ran 1 test (total:0.78s - test:0.71s)
[good] tests.py::py3_flake8 [default-linux-x86_64-release] (0.00s)
Logsdir: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/it2/test-results/flake8.py3/testing_out_stuff
------ GOOD: 1 - GOOD alice/hollywood/library/scenarios/foo_bar/it2

alice/hollywood/library/scenarios/foo_bar/it2 <import_test>
------ sole chunk ran 1 test (total:2.27s - test:2.23s)
[good] alice-hollywood-library-scenarios-foo_bar-it2::import_test [default-linux-x86_64-release] (1.85s)
Logsdir: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/it2/test-results/import_test/testing_out_stuff
Stdout: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/it2/test-results/import_test/testing_out_stuff/alice-hollywood-library-scenarios-foo_bar-it2.import_test.out
------ GOOD: 1 - GOOD alice/hollywood/library/scenarios/foo_bar/it2

alice/hollywood/library/scenarios/foo_bar/ut <unittest>
------ sole chunk ran 2 tests (total:0.68s - test:0.61s)
[good] FooBarRender::FooBarRender1 [default-linux-x86_64-release] (0.00s)
Logsdir: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/ut/test-results/unittest/testing_out_stuff
Stderr: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/ut/test-results/unittest/testing_out_stuff/FooBarRender.FooBarRender1.err
Stdout: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/ut/test-results/unittest/testing_out_stuff/FooBarRender.FooBarRender1.out
[good] FooBarDispatch::FooBarDispatch1 [default-linux-x86_64-release] (0.00s)
Logsdir: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/ut/test-results/unittest/testing_out_stuff
Stderr: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/ut/test-results/unittest/testing_out_stuff/FooBarDispatch.FooBarDispatch1.err
Stdout: /home/mihajlova/arcadia/alice/hollywood/library/scenarios/foo_bar/ut/test-results/unittest/testing_out_stuff/FooBarDispatch.FooBarDispatch1.out
------ GOOD: 2 - GOOD alice/hollywood/library/scenarios/foo_bar/ut

Total 3 suites:
	3 - GOOD
Total 4 tests:
	4 - GOOD
Ok
```
{% endcut %}

## Графы и конфиги сценария

[Конфиги](megamind/config.md) сценария можно откорректировать или создать вручную.

{% note warning %}

Надо создать **все 4** конфига сценария.
Не забудьте выставить корректный `BaseUrl` для конкретной конфигурации. 

{% endnote %}

Ниже приведен пример минимального корректного конфига для [dev](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/dev).

Имя файла `MyTestScenario.pb.txt`
```yaml
   Name: "MyTestScenario"
   Description: "Описание сценария"
   Languages: [
       L_RUS
   ]
   DataSources: [
       {
           Type: USER_LOCATION
       }
   ]
   AcceptedFrames: [
   ]
   Handlers: {
       BaseUrl: "http://scenarios.hamster.alice.yandex.net/my_test_scenario/"
       Tvm2ClientId: "2000464"
       OverrideHttpAdapterReqId: true
       RequestType: AppHostPure
   }
   Enabled: False

   # Обязательно надо указать хотя бы один abc сервис
   Responsibles {
       Logins: "login"
       AbcServices {
           Name: "myservice"
       }
   }
   ```

Далее, с помощью [скрипта](https://a.yandex-team.ru/arc/trunk/arcadia/alice/apphost/graph_generator), надо обновить аппхостовые графы.

Будет добавлен шаблонный граф сценария в [директорию с графами Алисы](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE), обновлены [_custom_alerts.json](https://a.yandex-team.ru/arc/trunk/arcadia/apphost/conf/verticals/ALICE/_custom_alerts.json) и аппхостовый граф Megamind.

{% note warning %}

Если шаблонный вариант не подходит, то откорректируйте файл с графом. Не рекомендуется изменять общую структуру файла и ответственных в ручном режиме.

{% endnote %}

Все изменения можно посмотреть с помощью команды 
```
arc diff arcadia/apphost/conf/verticals/ALICE
```

{% note alert %}

По умолчанию создается новый двухнодовый граф. Если нужен старый вариант графов, добавьте ```–d, --deprecated```.

{% endnote %}


## Тестирование сценария

Способы тестирования:
* unit тесты на ``С++``: 
* it2 тесты
* EVO тесты
* ручное тестирование

#### NLG
Не забудьте поправить `nlg` для своего сценария. В этом поможет [инструкция](https://wiki.yandex-team.ru/alice/hollywood/scenarios-in-hollywood/nlginhollywood/) и примеры других сценариев.

Если не нужен, удалите папку с `nlg`.

#### Unit тесты
Создаются вместе со сценарием. При необходимости редактируются под нужды автора сценария самостоятельно.

Вся актуальная документация доступна [здесь](./hollywood/ut/intro.md).

#### IT2 тесты
Создаются вместе со сценарием. При необходимости редактируются под нужды автора сценария самостоятельно.

Вся актуальная документация доступна [здесь](https://wiki.yandex-team.ru/alice/hollywood/integrationtests/).

#### EVO тесты
EVO тесты находятся [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/alice/tests/integration_tests).

Для запуска их с локальными изменениями используйте флаги `-DMM -DHW -DAPP` или `-DALL`.
Подробнее [тут](https://a.yandex-team.ru/arc/trunk/arcadia/alice/tests/integration_tests/readme.md#zapusk-testov-dlya-testirovaniya-lokalьnyh-izmenenij)

#### Ручное тестирование
Все вышеперечисленные способы умеют поднимать локально сервисы, если им это потребуется. При ручном тестировании это необходимо сделать самостоятельно.

##### Первый шаг – поднять сервисы. 
###### Ручной вариант
Поднять только Hollywood.
1. Необходимо закоммитить изменения в конфига и графах. 

    До `http://megamind-ci.alice.yandex.net/` конфиги докатываются через 15-20 минут, а графы — 5-10 минут (проверить можно [тут](https://horizon.z.yandex-team.ru/revisions?textFilter=megamind)). Если коммитить графы сразу не хочется, то есть возможность запустить apphost локально по [инструкции](testing/srcrwr#srcrwr-apphost).

2. Подготовьте сервер: убедитесь, что машина, на которой вы хотите запустить сценарий, доступна из макроса `_GENCFG_BASSPRODNETS_`, чтобы до нее доходили запросы от Мегамайнда.

   Персональные виртуальные машины QYP из макроса `_ALICEDEVNETS_` доступны Мегамайнду по умолчанию.

3. Собрать и запустить hollywood `ya make alice/hollywood && ./scripts/run/run-hollywood`. Ваш сценарий будет отвечать мегамайнду на порту 12346. Именно этот порт надо указывать дальше для тестов.

###### Автоматизированный вариант
Поднять всё локально. Для этого можно воспользоваться скриптом [alice/hollywood/hw](https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/hw)


{% note alert %}

Это новый способ. Пока возможны проблемы и потребуются доработки. Пишите в Megamind Support.

{% endnote %}

###### Сборка
Скрипт по-умолчанию не собирает необходимые для работы бинари. Их нужно собрать самостоятельно, либо руками, либо воспользоваться специальным режимом бинаря.
```
ya make -rA -DHW  # соберёт только Hollywood
ya make -rA -DMM  # соберёт только Megamind
ya make -rA -DAP  # соберёт только AppHost
ya make -rA -DUA  # соберёт только UnifiedAgent, он так же собирается с флагами DHW, DMM

ya make -rA -DHW -DMM -DAP  # соберёт все необходимые бинари
ya make -rA -DALL  # соберёт все необходимые бинари
```

###### Запуск

```
ya make -r && ./hw run
```

```bash
usage: hw run [-h] [-s SCENARIO] [shard]

positional arguments:
  shard                 Hollywood shard (default: all)

options:
  -h, --help            show this help message and exit
  -s SCENARIO, --scenario SCENARIO
                        Hollywood shards (default: [])
```

Локально поднимется вся связка ```apphost+mm+hw```. В первой строчке лога будет выведен ```megamind url```, который можно использовать для переопределения (в том числе и для EVO тестов):
```
INFO MEGAMIND URL: http://omi.sas.yp-c.yandex.net:40004/speechkit/app/pa/?srcrwr=MEGAMIND_ALIAS:omi.sas.yp-c.yandex.net:12401&srcrwr=HOLLYWOOD_ALL:omi.sas.yp-c.yandex.net:12346
```
Логи HW и MM будут лежать в папке ```logs```: 
```bash
~/arcadia/alice/hollywood/hw$ ls logs/
hollywood-log  hollywood_server.err  hollywood_server.out  megamind-log  megamind_server.err  megamind_server.out  storage
```

{% cut "Полный вывод" %}
```bash
2022-05-20 18:17:00 omi.sas.yp-c.yandex.net root[533989] INFO MEGAMIND URL: http://omi.sas.yp-c.yandex.net:40004/speechkit/app/pa/?srcrwr=MEGAMIND_ALIAS:omi.sas.yp-c.yandex.net:12401&srcrwr=HOLLYWOOD_ALL:omi.sas.yp-c.yandex.net:12346
2022-05-20 18:17:00 omi.sas.yp-c.yandex.net root[533989] INFO Starting AppHost: ['/home/mihajlova/arcadia/apphost/tools/app_host_launcher/app_host_launcher', 'setup', '--nora', '--port', '40000', '--tvm-id', '2000860', '--force-yes', '--install-path', 'local_app_host_dir/', 'arcadia', '--vertical', 'ALICE', '--configuration', 'ctype=test;geo=sas']
2022-05-20 18:17:00 omi.sas.yp-c.yandex.net root[533989] INFO Starting Megamind: ['/home/mihajlova/arcadia/alice/megamind/scripts/run/run', '-c', '/home/mihajlova/arcadia/alice/megamind/configs/dev/megamind.pb.txt', '-p', '12398', '--logs', 'logs', '--run-unified-agent', '--service-sources-black-box-timeout-ms=1000']
2022-05-20 18:17:00 omi.sas.yp-c.yandex.net root[533989] INFO Starting Hollywood: ['/home/mihajlova/arcadia/alice/hollywood/scripts/run/run-hollywood-bin', '-c', '/home/mihajlova/arcadia/alice/hollywood/shards/all/dev/hollywood.pb.txt', '--app-host-config-port', '12345', '--logs', 'logs', '--run-unified-agent']
2022-05-20 18:17:00 omi.sas.yp-c.yandex.net alice.library.python.utils.network[533989] INFO Waiting for AppHost to start listening the port 40000...
2022-05-20 18:17:01,471 INFO:Creating local folders in /home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE.
2022-05-20 18:17:01,474 INFO:Parallel building dependencies ['app_host', 'event_log_dump']. Based on local cache state, this may take from few seconds up to ten+ minutes.
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533994] INFO Starting unified_agent
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533994] INFO Running command: /home/mihajlova/arcadia/logbroker/unified_agent/bin/unified_agent --config /home/mihajlova/arcadia/alice/megamind/deploy/nanny/dev_unified_agent_config.yaml --log-file /home/mihajlova/arcadia/alice/megamind/logs/unified_agent.err
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533994] INFO Started server unified_agent, pid: 534002
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533994] INFO Starting server: /home/mihajlova/arcadia/alice/megamind/server/megamind_server, args: Namespace(use_local=False, use_gdb=False, server=PosixPath('/home/mihajlova/arcadia/alice/megamind/server/megamind_server'), logs=PosixPath('logs'), arcadia_dir=PosixPath('/home/mihajlova/arcadia'), source_dir='/home/mihajlova/.ya/megamind_data', vins_package_abs_path=None, run_unified_agent=True, ua_binary=PosixPath('/home/mihajlova/arcadia/logbroker/unified_agent/bin/unified_agent'), ua_config=PosixPath('/home/mihajlova/arcadia/alice/megamind/deploy/nanny/dev_unified_agent_config.yaml'), ua_logs=PosixPath('/home/mihajlova/arcadia/alice/megamind/logs'), ua_uri='localhost:12387', ua_log_file=PosixPath('/home/mihajlova/arcadia/alice/megamind/logs/unified_agent_backend.err')), argv: ['-c', '/home/mihajlova/arcadia/alice/megamind/configs/dev/megamind.pb.txt', '-p', '12398', '--service-sources-black-box-timeout-ms=1000']
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Starting unified_agent
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Running command: /home/mihajlova/arcadia/logbroker/unified_agent/bin/unified_agent --config /home/mihajlova/arcadia/alice/hollywood/scripts/nanny_files/unified_agent_config.yaml --log-file /home/mihajlova/arcadia/alice/hollywood/logs/unified_agent.err
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Started server unified_agent, pid: 534030
logbroker/unified_agent/common/state_storage.cpp:83: cant lock directory [logs/storage/.state], maybe other instance is running there, run file [534002;/home/mihajlova/arcadia/logbroker/unified_agent/bin/unified_agent --config /home/mihajlova/arcadia/alice/megamind/deploy/nanny/dev_unified_agent_config.yaml --log-file /home/mihajlova/arcadia/alice/megamind/logs/unified_agent.err;storage-logs_storage_setrace;9], prev run file [521438;/home/mihajlova/arcadia/logbroker/unified_agent/bin/unified_agent --config /home/mihajlova/arcadia/alice/megamind/deploy/nanny/dev_unified_agent_config.yaml --log-file /home/mihajlova/arcadia/alice/megamind/logs/unified_agent.err;storage-logs_storage_setrace;8]
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Starting hollywood_server
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Running command: /home/mihajlova/arcadia/alice/hollywood/shards/all/server/hollywood_server -c /home/mihajlova/arcadia/alice/hollywood/shards/all/dev/hollywood.pb.txt --fast-data-path /home/mihajlova/arcadia/alice/hollywood/shards/all/prod/fast_data --scenario-resources-path /home/mihajlova/arcadia/alice/hollywood/shards/all/prod/resources --common-resources-path /home/mihajlova/arcadia/alice/hollywood/shards/all/prod/common_resources --hw-services-resources-path /home/mihajlova/arcadia/alice/hollywood/shards/all/prod/hw_services_resources --app-host-config-port 12345 --rtlog-unified-agent-uri localhost:12384 --rtlog-unified-agent-log-file /home/mihajlova/arcadia/alice/hollywood/logs/unified_agent_backend.err
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Started server hollywood_server, pid: 534053
2022-05-20 18:17:01 omi.sas.yp-c.yandex.net root[533996] INFO Finished unified_agent
2022-05-20 18:17:01,899 INFO:Running AppHost in /home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE

Add cgi &srcrwr=APP_HOST:omi.sas.yp-c.yandex.net:40000:10000000&timeout=10000000&waitall=da to redirect request to local app_host

If you want to start app_host later, just copy next line to console:
cd /home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE; ./run.sh

To check APPHOST event_log, use following command:
/home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE/evlogdump /home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE/eventlog-40000

To check HTTP_ADAPTER event_log, use following command:
/home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE/evlogdump /home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE/http_adapter_logs/eventlog-http_adapter-40004
Adapter access/requests logs also could be found in /home/mihajlova/arcadia/alice/hollywood/hw/local_app_host_dir/ALICE/http_adapter_logs


Press Esc button to stop app_host.

2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Getting info about sandbox resource id=2077646575
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Done
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Getting info about sandbox resource id=3042244191
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Done
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Getting info about sandbox resource id=930911826
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Done
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Getting info about sandbox resource id=1746761506
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Done
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Getting info about sandbox resource id=1791041272
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Done
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Getting info about sandbox resource id=3069941764
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Done
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Starting megamind_server
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Running command: /home/mihajlova/arcadia/alice/megamind/server/megamind_server -c /home/mihajlova/arcadia/alice/megamind/configs/dev/megamind.pb.txt -p 12398 --service-sources-black-box-timeout-ms=1000 --rtlog-unified-agent-uri localhost:12387 --rtlog-unified-agent-log-file /home/mihajlova/arcadia/alice/megamind/logs/unified_agent_backend.err --partial-pre-classification-model-path /home/mihajlova/.ya/megamind_data/resources/partial_preclf_model.cbm --formulas-path /home/mihajlova/.ya/megamind_data/formulas/megamind_formulas --geobase-path /home/mihajlova/.ya/megamind_data/geodata6.bin
2022-05-20 18:17:02 omi.sas.yp-c.yandex.net root[533994] INFO Started server megamind_server, pid: 534059
ok
=========== Run ===========
2022-05-20 18:17:03 omi.sas.yp-c.yandex.net alice.library.python.utils.network[533989] INFO Service AppHost listens the port 40000
2022-05-20 18:17:03 omi.sas.yp-c.yandex.net alice.library.python.utils.network[533989] INFO Waiting for Megamind to start listening the port 12398...
2022-05-20 18:17:04 omi.sas.yp-c.yandex.net alice.library.python.utils.network[533989] INFO Service Megamind listens the port 12398
2022-05-20 18:17:04 omi.sas.yp-c.yandex.net alice.library.python.utils.network[533989] INFO Waiting for Hollywood to start listening the port 12345...
2022-05-20 18:17:46 omi.sas.yp-c.yandex.net alice.library.python.utils.network[533989] INFO Service Hollywood listens the port 12345
```
{% endcut %}

##### Следующий шаг – сделать запрос.

Это можно сделать с помощью бота [AmandaJohnson](testing/amanda.md).
1. Авторизуйтесь командой `/login`.
1. Отправьте команду настройки megamind url `/setmmurl`.
1. Задайте адрес: `http://megamind-ci.alice.yandex.net/speechkit/app/pa/`. С помощью команды `/addqueryparam` добавьте параметр `srcrwr=NODE:<хост:порт>`. Порт для сценария под голливудом нужно указать `12346`.

      Например:
      ```
      http://megamind-ci.alice.yandex.net/speechkit/app/pa/?srcrwr=MYTESTSCENARIO:test-vm.sas.yp-c.yandex.net:12346
      ``` 
      Тут `MYTESTSCENARIO` — нода, которую мы хотим переопределить. Это либо название вершины в вашем графе из п.2, либо алиасы, указанные в секции `alias_config.addr_alias` этой вершины.
      Подробнее про `srcrwr` можно прочитать в [доке](testing/srcrwr.md).
1. Отправьте команду для указания экспериментов `/experiments`.
1. Задайте строку, которая включит сценарий для Мегамайнда: `mm_enable_protocol_scenario=MyTestScenario`.

Отправьте боту какое-нибудь текстовое сообщение: в логах hollywood появятся записи про пришедший запрос.

## Контактная информация

Если у вас появились вопросы, их можно задавать в [чаты поддержки](faq.md#support) соответствующих компонент.
