# Описание

Xiaomi Adapter для [Smart Home API](https://yandex.ru/dev/dialogs/smart-home/)

# Платформа:

Бекенд в RTC: [nanny](https://nanny.yandex-team.ru/ui/#/services/) -> quasar -> xiaomi_adapter <br>
Сборка образов: [QUASAR_IOT_XIAOMI_ADAPTER_DOCKER_BUILD](https://testenv.yandex-team.ru/?screen=job_history&database=alice-iot&job_name=QUASAR_IOT_XIAOMI_ADAPTER_DOCKER_BUILD)

## Сборка локально

### Пререквизиты

При сборке через `ya make` используются не встроенные в Go либы, а контрибы с Аркадии, так что их надо скачать заранее:

```(sh)
cd ~/arcadia
ya make --checkout -j0 contrib/go
```

### Просто собрать бинарь с сервером

```(sh)
cd ~/arcadia/alice/iot/adapters/xiaomi_adapter
ya make
```

Бинарь `server` появится в `~/arcadia/alice/iot/adapters/xiaomi_adapter/cmd/server/`

### Собрать Docker образ

Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.

```
cd ~/arcadia/alice/iot/adapters/xiaomi_adapter
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```

Будет собран образ `registry.yandex.net/quasar/xiaomi_adapter:{revision}`, где `{revision}` - текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:

```
cd ~/arcadia/alice/iot/adapters/xiaomi_adapter
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

# Документация на Xiaomi Adapter Smart Home API

Адреса апи:
* https://quasar-xiaomi-adapter.yandex.net -- для внутренней

Документация от Xiaomi: <br>
[IOT Account Hub](https://iot.mi.com/) <br>
[Development Center](https://dev.mi.com/) <br>
[Xiaomi Oauth2](https://dev.mi.com/console/doc/detail?pId=897) <br>
[Xiaomi IoT Control Terminal API](https://iot.mi.com/new/doc/guidelines-for-access/other-platform-access/control-api) <br>
[miot spec example link](http://miot-spec.org/miot-spec-v2/instance?type=urn:miot-spec-v2:device:light:0000A001:philips-bulb:1)

Устарело:<br />
[~~Xiaomi IoT protocol specification~~](https://iot.mi.com/new/guide.html?file=07-%E4%BA%91%E5%AF%B9%E4%BA%91%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/02-%E5%BA%94%E7%94%A8%E4%BA%91%E5%AF%B9%E4%BA%91%E6%8E%A5%E5%85%A5/01-%E5%B0%8F%E7%B1%B3IOT%E5%8D%8F%E8%AE%AE%E8%A7%84%E8%8C%83) <br />
[~~Xiaomi IoT cloud-to-cloud API~~](https://iot.mi.com/new/guide.html?file=07-%E4%BA%91%E5%AF%B9%E4%BA%91%E5%BC%80%E5%8F%91%E6%8C%87%E5%8D%97/02-%E5%BA%94%E7%94%A8%E4%BA%91%E5%AF%B9%E4%BA%91%E6%8E%A5%E5%85%A5/02-%E5%B0%8F%E7%B1%B3IOT%E6%8E%A7%E5%88%B6%E7%AB%AFAPI) <br />
[~~Debugger~~](https://debugger.iot.mi.com/debugger) <br>
