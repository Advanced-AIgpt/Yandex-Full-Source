Бот для cбора данных для обучения Алисы в режиме Wizard-of-Oz
====================================================

## Как запустить сервера

* Получить token у BotFather и указать его в config.yaml.
* Указать, в какую таблицу сохранять диалоги пользователей.
* Установить библиотеку python-telegram-bot.

```
pip install -r requirements.txt
```

## Как пользоваться ботом

### Запуск бота

Для запуска бота надо подготовить `config.yaml` и запустить его в командной строке:


```bash
python3.9 bot.py --config config.yaml
```

Пример `config.yaml`:

```yaml
tg_token: "..."
result_table_yt_path: "//path/to/result"
task_table_yt_path: "//path/to/task/table"
unique_links_table_yt_path: "//path/to/links/table"
staff_token: "AQAD-..."
yt_proxy: "hahn"
telegram_url_prefix: "https://t.me/<bot_name>?start="
admins: [<your_superheroes_names>]
```

### Requirements

Нужны три таблицы:

- `result_table_yt_path` -- для записи результатов
- `task_table_yt_path` -- таблица с историями пользователей
- `unique_links_table_yt_path` -- таблица с уникальными номерами

Пример `task_table_yt_path`:

```json
[
    {
        "user_story": "Я принцесса",
        "user_task": "Поговорите с Алисой о сказках"
    },
    {
        "user_story": "Я маленькая двеочка",
        "user_task": "Поговорите с Алисой о путешествиях"
    }
]
```

Пример `unique_links_table_yt_path`:

```json
[
    {
        "uuid": "https://t.me/<bot_name>?start=380c2089-2515-4be2-b643-ea614fbeb25f"
    },
    {
        "uuid": "https://t.me/<bot_name>?start=3d49e93a-4808-4696-8da0-acd33e20dd1b"
    },
    {
        "uuid": "https://t.me/<bot_name>?start=119f038f-6cd9-4481-b52d-ea2597042db0"
    },
    {
        "uuid": "https://t.me/<bot_name>?start=2"
    }
]
```

Для начала диалога пользователь должен перейти по ссылке в формате:

```https://t.me/{bot_name}?start={uuid}```

`bot_name` -- имя бота.

`uuid` -- уникальный номер

При простом переходе в бот придет сообщение о неправильном входе.

### Как сохраняются логи

Логи сохраняются следующим образом


```json
[
    {
        "dialog_id":"380c2089-2515-4be2-b643-ea614fbeb25f-119f038f-6cd9-4481-b52d-ea2597042db0",
        "dialog":["Пользователь: Син","Алиса: Синдарела"],
        "alice_id": 1,
        "user_id": 2,
        "user_story": "Я принцесса"
    },
    {
        "dialog_id":"c2b8d887-eca3-428f-920c-85265999e65b-3d49e93a-4808-4696-8da0-acd33e20dd1b",
        "dialog":["Пользователь: Где находится мама?","Алиса: включаю поиск"],
        "alice_id": 10,
        "user_id": 12,
        "user_story": "Я маленькая девочка"
    },
]
```

### Другие команды пользователя:

- `/private` -- отправить нелогируеммое сообщение другому пользователю. 
- `/dialog_id` -- получить номер диалога
- `/stop` -- остановить диалог
- `/stop_search` -- остановить поиск собеседника

### Админка

У бота есть возможность назначать админов. Это делается через `config.yaml` с помощью переменной `admins`.

Команды админа:

- `/add_user` -- добавить пользователя, которого нет в базе
- `/current_num_dialogs` -- сколько диалогов сейчас пишутся
- `/change_unique_links_table_path` -- изменить уникальные ссылки
- `/change_task_table_path` -- изменить задания пользователей
- `/stop_dialog` -- остановить диалог по id


## Тесты

Для бота есть специальные тесты, которые могут проверить его функциональность. Для их запуска потребуется `pytest` и `pytest-mock`.

Запуск тестов:

```bash
pytest tests\
```
