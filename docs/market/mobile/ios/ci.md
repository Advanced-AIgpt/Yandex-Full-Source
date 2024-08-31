# CI / CD

{% note warning %}

На текущий момент существует проблема со стабильностью прогона ui-тестов на arm-агентах (процессоры M1). Скролл в симуляторе работает нестабильно, пробуем решить в рамках [MMG-1](https://st.yandex-team.ru/MMG-1). В случае, если есть стабильно флакающие тесты, есть смысл их отключить в [project.yml](https://a.yandex-team.ru/arcadia/mobile/market/ios/app-market/project.yml), привязав тикет на починку к [BLUEMARKETAPPS-45199](https://st.yandex-team.ru/BLUEMARKETAPPS-45199).

{% endnote %}

### Текущий стек 

На данный момент для сборки и прогона тестов используются следующие инструменты:

- [Sandbox](https://sandbox.yandex-team.ru/admin/groups/MARKET_IOS_B2C/general) - распределенная система для выполнения пользовательских задач. [Документация](https://docs.yandex-team.ru/sandbox/);
- [Teamcity Sandbox Runner](https://docs.yandex-team.ru/teamcity-sandbox-runner/) - надстройка над Sandbox, позволяющая гибко управлять мобильными сборками;
- [Teamcity](https://teamcity.yandex-team.ru/project/MobileNew_Monorepo_YandexMarketIOS) - в текущей схеме используется только для конфигурирования sandbox-задач и проксирования артефактов сборки.
- [ЦУМ](https://tsum.yandex-team.ru/pipe/projects/mobile-blue/releases/active) - здесь запускаются и работают релизные пайплайны.

### Джобы

 - [build_for_qa](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_BuildForQa) - сборка `Inhouse` и последующая дистрибьюция в `Yandex Beta` с отбивкой в тикет;
 - [build_for_store](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_BuildForStore) - сборка релизной сборки приложения и загрузка её в `AppConnect`;
 - [build_for_tests](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_BuildForTests) - сборка схемы `AllTests` и архивирование артефактов сборки;
 - [run_unit_tests](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_RunUnitTests) - загрузка артефактов `build_for_tests` и запуск unit-тестов с генерацией junit отчета;
 - [run_all_tests](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_RunAllTests) - загрузка артефактов `build_for_tests` и запуск всех тестов с генерацией junit отчета;
 - [run_linter](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_RunLinter) - прогон `swiftlint` и `swiftformat` и генерация комбинированного репорта в артефакты джобы;
 - [build_for_flex_tests](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_BuildForFlexTests) - сборка приложения для flex unit тестов;
 - [run_flex_tests](https://teamcity.yandex-team.ru/buildConfiguration/MobileNew_Monorepo_YandexMarketIOS_RunFlexTests) - загрузка артефактов `build_for_flex_tests` и запуск flex snapshot тестов.

Большая часть данных джоб используется на проверках в пулл-реквестах арканиума, соответственно запускается из его интерфейса. Связь между чеком пулл-реквеста и проверкой в тимсити лежит в [a.yaml](https://a.yandex-team.ru/arcadia/mobile/market/ios/app-market/a.yaml)

### Особенности работы
После создания пулл-реквеста и перевода его в статус `Open`, запускаются соответствующие джобы в `Teamcity`, которые в свою очередь запускают задачи в `Sandbox`. Вся "полезная" работа выполняется в задачах `Sandbox`, ссылка на задачу `Sandbox` появляется через некоторое время в интерфейсе задачи `Teamcity`. 

{% note info %}

В интерфейсе `Teamcity` сборка практически все время будет в статусе `Waiting for sandbox task...` с иконкой песочных часов. Это абсолютно нормально, так как джоба ожидает результат выполнения задачи в `Sandbox`.

{% endnote %}

### Как читать логи
Ошибки пробрасываются на вкладку `Sandbox execution log` на страничке билда `Teamcity`. Полные же логи можно найти в `Sandbox`, переключившись на "сломавшуюся" задачу на табе `Child tasks`. Там в табе `Logs` можно посмотреть полный вывод задачи.

### Как завести новую джобу
- В `Teamcity` cкопировать существующую конфигурацию джобы с новым именем;
- Создать счетчик в `locke` квоте. Для в [IDM](https://idm.yandex-team.ru/) запросить роль для `YT кластер locke` с `Read and Write Access`. В качестве пути  указать `//home/yandex_market_ios`. Далее получить свой `YT token` [здесь](https://oauth.yt.yandex.net/) и выполнить скрипт ниже, где `<counter name>` это имя счетчика:
```bash
export YT_PROXY=locke
export YT_TOKEN=<YT token>
export QUOTA_NAME=yandex_market_ios
export BUILD_COUNTER_NAME=<counter name>
yt create uint64_node //home/$QUOTA_NAME/build_counters/$BUILD_COUNTER_NAME
yt set //home/$QUOTA_NAME/build_counters/$BUILD_COUNTER_NAME 1
```
- Проверить, что создался счетик с верным именем можно [здесь](https://yt.yandex-team.ru/locke/accounts/general?sortState=asc-false,field-alerts&account=yandex_market_ios).
- Создать новый конфиг `TSR` по [пути](https://a.yandex-team.ru/arcadia/mobile/market/ios/app-market-buildscripts/sandbox_configs). Документация [Teamcity Sandbox Runner](https://docs.yandex-team.ru/teamcity-sandbox-runner/). В конфиг добавить `build_counter` с параметром `yandex_market_ios:<counter name>`.
- В параметрах сборки в `Teamcity` поменять `sandbox.config_path` на путь до нового конфига.
