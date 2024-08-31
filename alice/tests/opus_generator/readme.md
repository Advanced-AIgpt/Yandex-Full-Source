Программа помогает сгенерировать опусы для тестов. 

На вход подаются текстовые данные из файла или консоли. На выходе – `.opus` файлы.

Одна строчка – один текст для генерации опуса.

Как работает:
- создает файл с текстом и именами `.opus` файлов для генерации.
Текст транслителируется на английский для использования в качестве имени опуса.
- создает и запускает граф в Нирване, который используя аннотацию в файле генерирует опусы.
- результат работы графа скачивается и сохраняется в указанную папку
- загружает данные в `Sandbox` по требованию


# Параметры

```
usage: opus_generator [-h] -d DST_PATH [-q QUOTA] [-t OAUTH_TOKEN] [-u]
                      [input]

positional arguments:
  input                 Path to file

optional arguments:
  -h, --help            show this help message and exit
  -d DST_PATH, --dst-path DST_PATH
                        OPUS_FOLDER_NAME
  -q QUOTA, --quota QUOTA
                        Nirvana quota
  -t OAUTH_TOKEN, --oauth-token OAUTH_TOKEN
                        Nirvana OAuth token
  -u, --upload          Upload to Sandbox
```


**input**

Имя файла с текстами для опуса. Одна строчка – один текст для генерации опуса.

Если имя файла не указано, программа ожидает ввода из консоли. Завершения ввода – `Ctrl` + `D` 


**-d, --dst-path**

Имя папки куда будут сохранены сгенерированные опусы.

Если папки не существует, она будет создана.

Если папка сущестует, она будет перезаписана.


**-t, --oauth-token**

OAuth токен для Нирваны. Токен можно получить [здесь](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=637ca17604cb4dfa90b262952c00b1e9).

Задать токен можно явно через параметры, либо через через [~/.vhrc](https://a.yandex-team.ru/arc/trunk/arcadia/nirvana/valhalla/docs/reference/run_kwargs.md#oauth_token)


**-q, --quota**

Квота в Нирване, которая будет использована для загрузки данных и запуска графа.

Если квоту указать, то одни и те же данные не будут загружаться два раза.
[Подробнее](https://a.yandex-team.ru/arc/trunk/arcadia/nirvana/valhalla/docs/reference/data_from_str.md)

Если не указать, данные всегда загружаются.
Это связанно с ошибкой `504 Timeout` для `findData` при загрузке данных без указания квоты.


**-u, --upload**

Загружает папку с опусами в `Sandbox` вызывая `ya upload`
```
ya upload --type ALICE_VOICE_COMMANDS --do-not-remove --description "Alice voice commans: DST_PATH" DST_PATH
```


# Пример запуска

```
./opus_generator -q voice-core -ud volume_voice_commands example.txt
```
```
INFO: Nirvana workflow created

    https://nirvana.yandex-team.ru/flow/c5abbe9c-75bb-4313-95aa-e92ee65239c1/5f3cc42b-e286-4ea1-8163-fbe86cf34f7d

API: startWorkflow

API: startWorkflow  --->  OK

INFO: ya upload --type ALICE_VOICE_COMMANDS --do-not-remove --description "Alice voice commans: volume_voice_commands" volume_voice_commands
Created resource id is 1486128958
	TTL          : INF
	Resource link: https://sandbox.yandex-team.ru/resource/1486128958/view
	Download link: https://proxy.sandbox.yandex-team.ru/1486128958
```

**example.txt**
```
громче
сделай громче
погромче
увеличить громкость
тише
сделай потише
потише
уменьшить громкость
громкость 2
уровень громкости 9
поставь громкость 4
поставь громкость 4 из 10
поставь громкость 2 из 10
Какая сейчас громкость?
какой уровень громкости установлен
Выключи звук
Включи звук
громкость 80
громкость 11
громкость минус 2
Вруби на всю
```