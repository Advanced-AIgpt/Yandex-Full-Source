Memento - сервис работы с памятью Алисы

# Тестирование при разработке сценариев
Переопределение окружения мементо делается с помощью `SRCRWR` по `uniProxyUrl`, например:
```
"uniProxyUrl": "wss://uniproxy.alice.yandex.net/uni.ws?srcrwr=VOICE__MEMENTO_APPHOST:VOICE__MEMENTO_CI_APPHOST&srcrwr=VOICE__MEMENTO:VOICE__MEMENTO_CI"
```

# Использование мементо в сценариях
1. Добавить нужный свой объект в `proto/*.proto` файлы
2. При необходимости добавить объект по умолчанию, положив файл в папку `proto/defaults/**/<MessageName>.pb.txt`
3. Сценарий подписывается на интересующиего его ключи конфига ([см. пример](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/scenarios/AliceShow.pb.txt?rev=r9228703#L25)), мементо прилетает в датасорсах
4. Для сохранения из мементо нужно отправить [серверную директиву](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/protos/scenarios/directives.proto?rev=r9217063#L2418) с заполненными данными для сохранения. Данные будут перезаписаны в разрезе переданного ключа UserConfigs или аналогичного ключа поверхности.
5. (**Deprecated**) Сценарий может ходить напрямую в ручки `get_objects` для получения списка ключей (тело в соответствии с [протоколом](https://a.yandex-team.ru/arc/trunk/arcadia/alice/memento/proto/api.proto): `TReqGetUserObjects`) и в `update_objects` для обновления (тело в соответствии с [протоколом](https://a.yandex-team.ru/arc/trunk/arcadia/alice/memento/proto/api.proto): `TReqChangeUserObjects`).

## Окружения:
Прод: https://memento.alice.yandex.net/memento/... (tvmid: 2021572, apphost: VOICE__MEMENTO_APPHOST)
Приемка: https://paskills-common-testing.alice.yandex.net/memento/... (tvmid: 2021570, apphost: VOICE__MEMENTO_TESTING_APPHOST)
CI: https://paskills-common-testing.alice.yandex.net/memento-ci/... (tvmid: 2021570, apphost: VOICE__MEMENTO_CI_APPHOST)



## Начало разработки
```
brew update
brew install openjdk@17
sudo ln -sfn /usr/local/opt/openjdk/libexec/openjdk.jdk /Library/Java/JavaVirtualMachines/openjdk.jdk
```

## Тестирование командой
Чтобы протестировать мементо на приемке в UI нужно открыть URL:
[https://hamster.yandex.ru/quasar/account/news?srcrwr=DIALOGS_HOST:priemka.dialogs.alice.yandex.ru](https://hamster.yandex.ru/quasar/account/news?srcrwr=DIALOGS_HOST:priemka.dialogs.alice.yandex.ru)
![qr](https://qr2.yandex.net/?text=https://hamster.yandex.ru/quasar/account/news?srcrwr=DIALOGS_HOST:priemka.dialogs.alice.yandex.ru "QR")

[https://hamster.yandex.ru/quasar/account/show?srcrwr=DIALOGS_HOST:priemka.dialogs.alice.yandex.ru](https://hamster.yandex.ru/quasar/account/show?srcrwr=DIALOGS_HOST:priemka.dialogs.alice.yandex.ru)
![qr](https://qr2.yandex.net/?text=https://hamster.yandex.ru/quasar/account/show?srcrwr=DIALOGS_HOST:priemka.dialogs.alice.yandex.ru "QR")


## Docker

В `pkg.json` используется список файлов исключений, чтобы разделить в docker слои из внешних библиотек
и аркадийных, которые часто меняются (внутри содержат timestamp билда и ревизию).

Получить этот список можно выполнив в корне memento:
```
ya make && ls -1 memento | grep -v "\-[0-9]" | sed 's/\.dylib/.so/g'
```

Если выполнять на MacOS, то артефакт с `.dylib` нужно переименовать в `.so`

ВАЖНО!
В первом списке исключений в `pkg.json` файл `memento.jar` учитывается, во втором, где копируются только аркадийные артефакты memento.jar быть не должно, он копируется отдельным шагом.

## Логи
https://yt.yandex-team.ru/hahn/navigation?filter=memento&path=//home/logfeller/logs
