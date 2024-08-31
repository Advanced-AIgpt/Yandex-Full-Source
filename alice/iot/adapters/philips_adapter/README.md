# Описание

Philips Adapter для [Smart Home API](https://yandex.ru/dev/dialogs/smart-home/)

# Платформа:

Бекенд в RTC: [nanny](https://nanny.yandex-team.ru/ui/#/services/) -> quasar -> philips_adapter <br>
Сборка образов: [QUASAR_IOT_PHILIPS_ADAPTER_DOCKER_BUILD](https://testenv.yandex-team.ru/?screen=job_history&database=alice-iot&job_name=QUASAR_IOT_PHILIPS_ADAPTER_DOCKER_BUILD)

## Сборка локально

### Пререквизиты

При сборке через `ya make` используются не встроенные в Go либы, а контрибы с Аркадии, так что их надо скачать заранее:

```(sh)
cd ~/arcadia
ya make --checkout -j0 contrib/go
```

### Просто собрать бинарь с сервером

```(sh)
cd ~/arcadia/alice/iot/adapters/philips_adapter
ya make
```

Бинарь `server` появится в `~/arcadia/alice/iot/adapters/philips_adapter/cmd/server/`

### Собрать Docker образ

Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.

```
cd ~/arcadia/alice/iot/adapters/philips_adapter
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```

Будет собран образ `registry.yandex.net/quasar/philips_adapter:{revision}`, где `{revision}` - текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:

```
cd ~/arcadia/alice/iot/adapters/philips_adapter
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

# Документация на Philips Adapter Smart Home API

Адреса апи:
* https://quasar-xiaomi-adapter.yandex.net -- для внутренней
