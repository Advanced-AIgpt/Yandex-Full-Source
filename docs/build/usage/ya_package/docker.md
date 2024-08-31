# Сборка docker-образов

С помощью команды
```bash
ya package <package.json> --docker
```
можно собрать докер-образ – для этого в json-описание пакета нужно добавить `Dockerfile`, который будет использоваться для последующих вызовов ```docker build``` и ```docker-push```, сам файл не обязательно должен лежать рядом с json-описанием, но обязательно должен иметь `/Dockerfile` в `destination`, [например](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/package/tests/create_docker/package/package.json):

```json
{
    "source": {
        "type": "RELATIVE",
        "path": "Dockerfile"
    },
    "destination": {
        "path": "/Dockerfile"
    }
}
```
Перед вызовом команды ```docker build``` в ее рабочей директории производится сборка пакета и подготовка его содержимого, таким образом при формировании команд в `DockerFile` (`COPY` и подобных) можно рассчитывать на наличие файлов и структуру каталогов, описанных в json-описании.

*Внимание*
* По умолчанию пакет будет иметь тег равный ```registry.yandex.net/<repository>/<package name>:<package version>```
  * `registry.yandex.net` – реестр образов, задается через параметр ```--docker-registry```, который по умолчанию задан как `registry.yandex.net`;
  * `<repository>` – название репозитория для пакета, задается через ```--docker-repository``` и по умолчанию не имеет значения.
* Для успешной работы ```ya package --docker``` необходимо наличие установленного на хост-системе пакета с `docker`, а также права на его запуск;
* ```ya package --docker``` не выполняет действий по авторизации в реестре (команду ```docker login```) – предполагается, что пользователь заранее об этом позаботился.
* Если не задан параметр ```--docker-save-image```, то в архиве пакета будет только результат работы команд ```docker```.
* Бинари, собираемые в процессе работы ```ya package --docker```, по дефолту, будут собраны под вашу хост-платформу, а не под платформу, указанную в докере. Для изменения платформы, нужно явно указать платформу в поле ```target-platforms``` секции [build](json.md#build)

## Ключи для сборки docker-образов { #keys }
* `--docker-registry` указание реестра для публикации (по умолчанию `registry.yandex.net`)
* `--docker-repository` указание репозитория для образа (участвует в имени образа)
* `--docker-save-image` сохранение образа в виде отдельного файла в ахиве
* `--docker-push` публикация образа в реестр
* `--docker-network` узазание ключа `--network` для вызываемой под капотом команды ```docker build```
* `--docker-build-arg` указание ключа `--build-arg` для вызываемой под капотом команды ```docker build```; формат значения должен иметь вид `<key>=<value>`

## Публикация образов  { #publish }
При добавлении `--docker-push` происходит публикация образа в реестре. Для успешной публикации у пользователя должны быть права на этот пакет в `IDM`. Подробнее про особенности работы с `registry.yandex.net` можно почитать [тут](https://wiki.yandex-team.ru/qloud/docker-registry/).

## Сохранение образов { #save }
Если нужно сохранить полученный образ в файл (например, для последующей его загрузке с помощью ```docker load```), то необходимо запускать:
```bash
ya package --docker --docker-save-image
```
файл образа будет сохранен в архиве пакета.

## Особенности запуска через задачу YA_PACKAGE_2 { #sandbox }
docker-образ можно собрать с помощью задачи `YA_PACKAGE_2`, для чего предварительно в [Vault](https://sandbox.yandex-team.ru/admin/vault) необходимо положить OAuth-токен для авторизации в реестре, а в задаче заполнить поля, относящиеся к сборке docker-образов, выбрав `Package type` = `docker`:
* `Image repository` – репозиторий для образа (соответсвует ключу ```--docker-repository```)
* `Save docker image in resource` – сохранять или нет файл образа (соответсвует ключу ```--docker-save-image```)
* `Push docker image` – публиковать образ в реестре или нет (соответсвует ключу ```--docker-push```)
* `Docker registry` – докер-реестр (по умолчению: registry.yandex.net, соответсвует ключу ```--docker-registry```)
* `Docker user` – пользователь для команды ```docker login```
* `Docker token vault name` – название ключа в `Vault`, где хранится токен для команды ```docker login```

### Передача секрета в контейнер для docker build { #docker-build-secret }
В случае необходимости безопасно передать секрет внутрь docker container для его сборки, вы можете воспользоваться `--docker-build-arg=ENV_VAR` без указания значения, где ENV_VAR - переменная окружения, которая содержит необходимое значение секрета. См. так же документацию про [docker-build](https://docs.docker.com/engine/reference/commandline/build/#set-build-time-variables---build-arg).

Для того чтобы передать секрет в docker контейнер при сборке образа средствами `YA_PACKAGE_2`, нужно:
- Положить секрет в [Секретницу](yav.yandex-team.ru/) или [Sandbox Vault](https://sandbox.yandex-team.ru/admin/vault) (deprecated).
- Выставить в таске `env_vars` в [соответствующей нотации](https://a.yandex-team.ru/arcadia/sandbox/projects/common/build/parameters/__init__.py?rev=r9713568#L1509-1523), для того чтобы нужные переменные окружения содержали требуемый секрет. 
- Добавить в `docker_build_arg` нужные переменные окружения из `env_vars`, которые содержат секреты и в качестве значения указать `None`. В этом случае `ya package` возьмёт значение из окружения, которое было задано в `evn_vars`.