# <img alt="bulb" src="https://jing.yandex-team.ru/files/mavlyutov/pokemon_icon_208_01.png" width="220" />

# Описание

Сервис приема нотификаций об изменении состояния устройств<br>
Внешняя документация на [tech.yandex.ru](https://yandex.ru/dev/dialogs/smart-home/)

# Балансеры
## L3 Балансер
Принадлежит команде Диалогов: [dialogs.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/steelix-production.balancer.quasar.yandex.net/l3-balancers/list/dialogs.yandex.net/show/)
## L7 Балансер
Живет в awacs: [steelix-production.balancer.quasar.yandex.net](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/steelix-production.balancer.quasar.yandex.net/show/)
## Регулировка весов в L7-Heavy
Регулировать веса распределения трафика по ДЦ можно в [интерфейсе l7-heavy](https://nanny.yandex-team.ru/ui/#/l7heavy/steelix-production.balancer.quasar.yandex.net/
)
# Разработка

## Просто собрать бинарь

```(sh)
cd ~/arcadia/alice/iot/steelix
ya make
```

Бинарь `steelix` появится в `~/arcadia/alice/iot/steelix/cmd/server/`

## Локальная разработка
1. Сохраняем секрет TVM-приложения [iot-steelix-testing](https://abc.yandex-team.ru/services/alice_iot/resources/?show-resource=8462066) в файл `$HOME/.tvm/2016205.secret`
2. Запускаем `tvm-tool` при помощи скрипта `./misc/local/tvm.sh`
3. Запускаем `steelix`:
   - выставляем `ENV_TYPE=local-*`, чтобы подключился конфиг для локальной разработки,
   - указываем путь к папке с конфигами
   ```(sh)
   cd ~/arcadia/alice/iot/steelix
   ENV_TYPE=local-priemka ./cmd/server/steelix -C ./config
   ```

# Сборка образа

## В Sandbox
Образец таски: [YA_PACKAGE(alice/iot/steelix/pkg.json)](https://sandbox.yandex-team.ru/task/536920568/view). Её нужно склонировать и запустить

## Автоматическая на каждый коммит
* Таска в TestEnv: [QUASAR_IOT_STEELIX_DOCKER_BUILD](https://testenv.yandex-team.ru/?screen=job_history&database=alice-iot&job_name=QUASAR_IOT_STEELIX_DOCKER_BUILD)
* Исходники: [/arc/trunk/arcadia/testenv/jobs/quasar/SteelixBuild.py](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/jobs/quasar/SteelixBuild.py)

## Локальная
Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.
```(sh)
cd ~/arcadia/alice/iot/steelix
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```
Будет собран образ `registry.yandex.net/alice/iot/steelix:{revision}`, где `{revision}` — текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:
```
cd ~/arcadia/alice/iot/steelix
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

Чтобы выкатить собранный образ, нужно дать ему уникальный тег и запушить его:
```(sh)
docker tag registry.yandex.net/alice/iot/steelix:{revision} registry.yandex.net/alice/iot/steelix:{revision}-{description}
docker push registry.yandex.net/alice/iot/steelix:{revision}-{description}
```
Здесь `{description}` — ключ или описание задачи, например `IOT-1-v1`

# Нагрузочное тестирование
Чтобы пострелять в Steelix, используйте готовые инструменты:
1. Генерируем патроны, с запросами, характерными для Steelix:
   1. Получаем OAuth-токен [в продакшен-паспорте](https://oauth.yandex.ru/authorize?response_type=token&client_id=c473ca268cd749d3a8371351a8f2bcbd)
   2. Генерируем патроны
   ```(sh)
   cd ~/arcadia/alice/iot/steelix/misc/make_ammo
   python3 __init__.py -t <token>
   cat ammo
   ```
   При необходимости можно подстроить количество запросов каждого типа в ленте или ожидаемую задержку "апстрима" на такой запрос (параметр delay)
2. Поднимаем в YDeploy окружение для стрельбы:
   1. Деплоим окружение, чтобы создать поды:
      ```(sh)
      cd ~/arcadia/alice/iot/steelix/misc/deploy
      ya tool dctl put stage -c xdc stress.yaml
      ```
   2. Открываем [стейдж в YDeploy](https://deploy.yandex-team.ru/project/iot-steelix-stress). Копируем FQDN из Deploy unit с названием "mock" (заглушка)

      Подробнее про заглушку — см. в файле `arcadia/alice/iot/steelix/cmd/upstream-mock/README.md`
   3. Открываем в редакторе конфиг окружения `~/arcadia/alice/iot/steelix/misc/deploy/stess.yaml`. Указываем FQDN заглушки (mock) в полях `spec.deploy_units.stress.replica_set.replica_set_template.pod_template_spec.spec.pod_agent_payload.spec.workloads[0].env[*].value`:
      ```
      UPSTREAM_URL_DEFAULT=http://{mock FQDN}:8080/paskills
      UPSTREAM_URL_BULBASAUR=http://{mock FQDN}:8080/bulbasaur
      UPSTREAM_URL_DIALOGOVO=http://{mock FQDN}:8080/dialogovo
      ```
   4. Вносим изменения в код Steelix, собираем образ (см. Сборка образа). Указываем хэш в полях `spec.deploy_units.mock.images_for_boxes.mock_server.tag` и `spec.deploy_units.stress.images_for_boxes.steelix_server.tag`
   5. Деплоим обновлённое окружение (см. п1): `ya tool dctl put stage -c xdc stress.yaml`
3. Запускаем стрельбу в Лунапарке
   1. Открываем [стейдж в YDeploy](https://deploy.yandex-team.ru/project/iot-steelix-stress). Копируем FQDN из Deploy unit с названием "stress" (код стиликса).
   2. В поле `Мишень` указываем `{stress FQDN}:8080`
   3. Заливаем патроны из пункта 1
   4. ???
   5. PROFIT

## Логи
Хранятся [в YT](https://yt.yandex-team.ru/arnold/navigation?filter=steelix&path=//home/logfeller/logs), разбиты по стендам.
Логи основного приложения
[steelix-production-logs](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs/vsdev-steelix-production-logs)
и логи балансера (access-log)
[balancer-steelix](https://yt.yandex-team.ru/arnold/navigation?path=//home/logfeller/logs/vsdev-balancer-steelix)

# Примеры логов
[последние 15 минут](https://yql.yandex-team.ru/Operations/YREfjAuEI1ttwRjzK1VQ4v2nirkjpayhLklg8g2MkMc=)
