# <img alt="vulpix" src="https://jing.yandex-team.ru/files/norchine/uhura_2020-08-25T04%3A11%3A34.522791.jpg" width="220" />

# Описание

Сервис, отвечающий за сценарий голосового подключения устройств за ММ. 

# Разработка

## Просто собрать бинарь

```(sh)
cd ~/arcadia/alice/iot/vulpix
ya make
```

Бинарь `vulpix` появится в `~/arcadia/alice/iot/vulpix/cmd/server/`

## Локальная

Сборка осуществляется с помощью `ya package`, который на самом деле сперва запускает `ya make`, а затем создает окружение для сборки контейнера, используя описание из `pkg.json`.
```(sh)
cd ~/arcadia/alice/iot/vulpix
ya package pkg.json --docker --docker-repository=iot --target-platform=DEFAULT-LINUX-X86_64
```
Будет собран образ `registry.yandex.net/alice/iot/vulpix:{revision}`, где `{revision}` — текущая ревизия локального кода

Если нужно сразу запушить, то надо добавить параметр `--docker-push`:
```
cd ~/arcadia/alice/iot/vulpix
ya package pkg.json --docker --docker-repository=iot --docker-push --target-platform=DEFAULT-LINUX-X86_64
```

Чтобы выкатить собранный образ, нужно дать ему уникальный тег и запушить его:
```(sh)
docker tag registry.yandex.net/alice/iot/vulpix:{revision} registry.yandex.net/alice/iot/vulpix:{revision}-{description}
docker push registry.yandex.net/alice/iot/vulpix:{revision}-{description}
```
Здесь `{description}` — ключ или описание задачи, например `IOT-1-v1`
