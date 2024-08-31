# Teamcity

## Описание

[Ссылка](https://teamcity.yandex-team.ru/project.html?projectId=Mobile_BlueMarketAndroid&branch_Mobile_BlueMarketAndroid=__all_branches__)

Основная система CI/CD в мобилках Маркета.

## Как пользоваться

Есть 2 вида дизайна. Я покажу на старой версии, потому что новая не зашла.

{% cut "Виды дизайна" %}

Новый
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:21:59Z.58b5a64.png)

Старый
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:23:01Z.765e799.png)

Переключалка между дизайнами
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:24:31Z.48d7b75.png)

{% endcut %}

### Проект
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-19T15:07:32Z.da83c76.png)

### Конфигурация
![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-22T09:18:48Z.ba541c2.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:24:33Z.62d9bcb.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:26:15Z.98da37d.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:32:51Z.83fb0c6.png)

### Сборка

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:43:53Z.b95f4d8.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:45:48Z.1c44dbf.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:46:41Z.50b55ec.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:54:14Z.34c8c83.png)


### Запуск сборки

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:34:54Z.be6c708.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:37:18Z.226147a.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:38:25Z.d3d0178.png)

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:39:25Z.23b69e4.png)

Остановить сборку можно, нажав на
![alt text](https://jing.yandex-team.ru/files/apopsuenko/Data.a7fb40f.png)

### Как работают сборки

1. Сборка висит в очереди
2. Стартует сборка на Sandbox
3. Сборка на Sandbox выполнена
4. Копируются результаты на TC

{% note warning %}

То, что сборка висит в очереди, не говорит о том, что она не выполняется.
Так реализовано, что на TC сборка в очереди, но она уже выполняется на Sandbox

{% endnote %}

## Типы сборок

### build

Эта конфигурация проверяет, что приложение собирается

### unit-tests

Гоняются unit-тесты

### unit-tests with coverage

Гоняются unit-тесты + проверка покрытия кода unit-тестами.

Проверка покрытия делается так:
1. Прогоняются unit-тесты
2. Jacoco считает покрытие
3. Генерится html-отчёт
4. Цифры из отчёта кладутся в спкрипт
5. Скрипт выполняется на TC, записывая цифры в метрики TC
6. Метрики выводятся в ui
7. Ищется последний подсчёт покрытия для develop
8. С этимм покрытием сравнивается покрытие на ветке
9. Если меньше на сколько-то (цифра меняется ~ 0.1%), то сборка падает, ругаясь на низкое покрытие

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T10:51:05Z.e512113.png)

### detekt & checkstyle

Прогоняются статические анализаторы:
- detekt (для котлин)
- checkstyle (для Java)

### dependency graph validation

Валидируется граф зависимостей между модулями

### ui tests build

Собираются ui-тесты.
Происходит сборка apk-приложения + apk-тестов.
Эти apk потом качаются при прогоне ui-тестов.
Таким образом можно, если прогон упал, не пересобирать прилоение и тесты, а просто перепрогнать тесты.

### ui-tests run

Прогон ui-тестов.
Allure-отчёт не скачивается с sandbox, так как долго качается.

### base for beta

Сборка baseRelease для проверки специфичных для base-мест, которая ещё отправляется в beta.m.soft, откуда её могут скачать.
Можно запустить из пайпа

{% note alert %}

При запуске важно задать ветку, по которойможно будет найти сборку на beta.m.soft

{% endnote %}

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:07:52Z.8e6e7d2.png)

### qa for beta

Сборка qaRelease, которая ещё отправляется в beta.m.soft, откуда её могут скачать.

{% note alert %}

При запуске важно задать ветку, по которойможно будет найти сборку на beta.m.soft

{% endnote %}

![alt text](https://jing.yandex-team.ru/files/apopsuenko/2021-10-25T11:07:52Z.8e6e7d2.png)

### build for GP

Сборка baseRelease с подписанием apk в подписаторе.

### increment version

Повышение версии + коммит новой версии

### Другие

Другие сборки вам скорее всего не понадобятся.
Там есть специфичные прогоны, конфигурации для мониторингов и автоматического выполнения чего-либо.

## Полезности

- Все сборки release подписываются подписатором, так что имеют релизную подпись.
- VersionCode берётся по номеру сборки в тимсити. Он инкрементируется самой тимсити.
- Если ты не видишь свою ветку, то
Проверь, что здесь выбраны все ветки
![alt text](https://jing.yandex-team.ru/files/apopsuenko/Data.c8cf657.png)
Также, если ты только что запушил свою ветку, она может ещё не прорасти на ТС

## Чатики:
- [Чат Тимсити](https://t.me/joinchat/AAAAAECtIetSv3Z_sqhQrQ)
- [Подписатор](https://t.me/joinchat/AFCw71JqrZCGy0MqPxqbYQ)
- [Артифактори](https://t.me/joinchat/AAAAAAyctlTNpipQKwdlFA)

## Настройки (для инфры)

### Общее

Запуск сборок на Sandbox работает по [инструкции](https://docs.yandex-team.ru/teamcity-sandbox-runner/)

В проекте есть папка `sandbox_configs`, в которой лежат конфиги для сборок
