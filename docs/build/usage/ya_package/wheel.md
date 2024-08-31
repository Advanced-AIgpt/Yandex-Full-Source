# Сборка wheel-пакетов

Чтобы собрать wheel-пакет, нужно выполнить
```bash
ya package <package.json> --wheel
```
В json-конфиге пакета нужно описать структуру пакета со специальным скриптом `setup.py` в корне формирующим пакет, который будет вызван с параметром ```bdist_wheel```.
Для сборки python3 пакета, нужно к опции ```--wheel``` добавить опцию ```--wheel-python3```, чтобы получилось ```ya package <package.json> --wheel --wheel-python3``` .
Версию пакета достаточно указать в `package.json`, а в `setup.py` ее можно получить через переменную окружения `YA_PACKAGE_VERSION`:
```python
setup(
    <...>,
    version=os.environ.get("YA_PACKAGE_VERSION"),
)
```
Пример описания wheel-пакета можно найти по [ссылке](https://a.yandex-team.ru/arc/trunk/arcadia/devtools/ya/package/tests/create_wheel/data/package_wheel.json).

## Публикация пакета  { #publish }
Если в вызове ```ya package``` добавить параметр ```--publish-to <repo_addr>```, то после сборки пакета он будет опубликован с помощью вызова ```twine upload --repository-url <repo_addr>```, при этом ключи от репозитория будут запрошены в консоли.

## Особенности запуска через задачу YA_PACKAGE { #sandbox }
Для публикации из sandbox в таске `YA_PACKAGE` помимо адреса репозитория нужно указать vault-name для ключей, которые предварительно нужно сохранить в [SandBox Vault](https://sandbox.yandex-team.ru/admin/vault). Подробнее про ключи pypi-репозитория можно почитать [здесь](https://wiki.yandex-team.ru/pypi/).

Чтобы собрать пакет в sandbox с помощью `YA_PACKAGE`, задачу необходимо выполнить в контейнере, в котором установлены `setuptools`, `wheel`, `twine`, по умолчанию используется https://sandbox.yandex-team.ru/resource/1197547734.
