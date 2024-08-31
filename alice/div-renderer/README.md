[![oko health](https://oko.yandex-team.ru/badges/repo.svg?repoName=alice/div-renderer&vcs=arc)](https://oko.yandex-team.ru/arc/alice/div-renderer)

# Рендерилка дивов

Это бэкенд для аппхоста, который принимает на вход шаблон и данные для него и возвращает [дивный](https://doc.yandex-team.ru/divkit/overview/concepts/general.html) json.

## Начало работы

1. Проверяем версию ноды и npm, они должны быть `v16.13.2` и `6.14.5` соответственно.

```shell
node --version
npm --version
```

2. Если версии не совпадают - ставим правильные.

```shell
nvm install 16.13.2
npm i -g npm@6.14.5
```

3. Убедиться что стоит `protoc` версии выше `3.14.0`

```
protoc --version
```

4. Выполняем `npm run gen:project`

5. Заходим в [секретницу](https://yav.yandex-team.ru/secret/sec-01g4b5vn0hg8fcxgjcpyx8sknx/explore/version/ver-01g4b5vn60trbjc3yy54b099w9) и копируем ключи локально в файлик `.env` в корне проекта
```dotenv
SECRET_MAPS_API_KEY="ключ api_key"
SECRET_MAPS_SIGNING_SECRET="ключ signing_secret"
```

## Полезные скрипты

### Подготовка

```shell
npm ci
```

### Генерация протобуфа

```
npm run proto
```

### Автофикс линтера

```
npm run lint -- --fix
```

### Скриншот моков для тестов

```
npm test -- -u
```

### Прогнать unit-тесты

```
npm run ci:unit
```

### Сборка

```shell
npm run build
```

### Пересборка в режиме реального времени

Пересобирает только изменившиеся файлы

```shell
npm run build -- -w
```

### Запуск dev apphost'a

```shell
npm run dev:backend
```

### Запуск dev веб-сервера с json-ом

```shell
npm run dev:showcase
```

После этой команды будет поднят сервер на порту 8000 и по адресу `http://localhost:8000/t/<templateId>` будет отдаваться json со сверстанным шаблоном.
Данные для рендера можно передать следующими query параметрами:

-   `d`: заэнкоженный json с данными.
-   `f`: путь до файла с данными в папке `dev/samples`
    Пример:

```
http://localhost:8000/t/ExampleScenarioData?d=%7B%22hello%22%3A%22world%22%7D
```

### Как запустить карточку на девайсе

Нужен установленный [adb](https://developer.android.com/studio/command-line/adb).
Далее нужно с помощью тулзы centaur_test послать директиву show_view с карточкой на девайс

```
cd $(arc root)/smart_devices/tools/centaur_test
ya make
curl "http://localhost:8000/t/NewsTeaserData?wrap=centaur&f=news/teaser" -o /tmp/div2.json &&  ./centaur_test execute  --name show_view --payload /tmp/div2.json
```

ya make нужно запускать только один раз для компиляции бинарника

### Формат запросов

Во входных данных в запросе ожидается type render_data с протобуфом

### Полезно backend'еру

#### Вывод данных на экран девайса

Как убедиться что все компоненты отработали и данные приехали правильные, если шаблон верстки еще написан?
Правильно! Добавить в конфиг своего девайса эксп флаг `scenario_data_pretty_json=${tempalte_name}`, где `template_name` название темплейта, который до npm run lint -- --fix должен отработать :)
Например, если в конфиг добавить вот такой эксп флаг - `scenario_data_pretty_json=WeatherDayHoursData`, то при клике на виджет погоды на главной, приедет не верстка страницы погоды, а JSON с данными из ScenarioData.

### Проверить с помощью servant_client

Есть утилита [servant_client](https://docs.yandex-team.ru/apphost/pages/manual_requests) для тестирования аппхостовых бэкендов.
Можно убедиться, что сервис работает и даже посмотреть, что срендерится:

#### Устанавливаем servant_client

```
cd $(arc root)/apphost/tools/servant_client
ya make -r
```

#### Запускаем сервер

```shell
npm run dev:backend
```

#### Шлем запрос

```
$(arc root)/apphost/tools/servant_client/servant_client localhost:10000/render ./src/projects/example/templates/ExampleDiv/request.json --proto-to-json --pretty
```

### Как добавить новый шаблон

Все шаблоны лежат в папке `projects/{имя проекта}/templates`.
Файл с шаблоном должен содержать экспорт функции, которую надо добавить в файле `projects/{имя проекта}/index.ts`.

### Как добавить новый проект

1. скопировать `src/projects/example`
2. зарегестрировать диспетчер шаблонов в файле `src/index.ts`

### Обновление протобуфа описания

```shell
npm run proto
```

### Генерация патронов для стрельб

```shell
npm run dev:ammogen
```

запустит настоящий и проксирующий бэкенды. Если ходить в проксирующий бэкенд на 10000 порт, то на каждый запрос он будет складывать патроны в папку ammos.

### Как сделать первый коммит в рендерилку и увидеть результат для бекенд-разработчика Centaur

Краткое повторение предыдущего
На локальной машине:

1. Выполняем пункт [Начало работы](#начало-работы)
2. Собрать бекенд и зависимости `npm run ci`
3. Сбилдить бекенд `npm run build`
4. Поднять бекенд `npm run dev:showcase`
5. Установить [adb](https://developer.android.com/studio/command-line/adb)
6. Подключить девайс по проводу, разрешить в настройках подключение по adb - Система->Для разработчкиков->Отладка по USB - Toggle On
7. Запустить `adb` и убедиться что есть видимые девайсы командой `adb devices`
8. Запустить приложение `Centaur` на девайсе
9. Собрать утилиту `centaur_test`

```
   cd $(arc root)/smart_devices/tools/centaur_test
   ya make
```

9. Запустить `ExampleScenarioData` карточку на девайс

```
curl "http://localhost:8000/t/NewsTeaserData?wrap=centaur&f=news/teaser" -o /tmp/div2.json &&  ../../../smart_devices/tools/centaur_test/centaur_test execute  --name show_view --payload /tmp/div2.json
```
