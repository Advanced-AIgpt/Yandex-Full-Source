## Как писать свои nile-тесты

### Что такое тесты на nile

Тесты на nile-цепочки операций — это тестирование функций, которые на вход принимают какой-то поток данных (nile stream) и возвращают какой-то поток данных.

Т.е. на вход функции можно подать `job.table(...)` а получить в `return` nile поток.

В самом тесте подменяется вход и выход операции, где дальше с выходом можно сделать что угодно, например сравнить с каноническим выходом.

Такие тесты удобны тем, что, в отличие от unit-тестов на python функции, проверяют цепочку работы найла end-to-end, включая project, map и другие операции

Данные на вход, и ожидаемые данные на выходе можно подать в виде файла с массивом из json объектов

Подробнее про локальный запуск [в документации nile](https://logos.yandex-team.ru/statbox/nile/advanced_tutorial/debug.html?highlight=local_run)

### Как написать новый тест на функцию с цепочкой операций nile
1. Проверить, что тестируемый код принимает на вход nile stream и возвращает nile stream. Если это не так, то код придётся декомпозировать
2. Скопировать любой файл с тестами на найле сюда же под новым именем
3. Создать одноимённую папочку, куда нужно будет сложить входные и ожидаемые данные
4. Чтобы создать входные данные для теста, нужно иметь: таблицу с исходными данными на YT, аркадию (или утилиту `ya`) и утилиту [jq](https://stedolan.github.io/jq/) для обработки json'а
    * считать табличку с YT в локальный файл можно такой командой:
    ```
    ya tool yt read '//home/blablabla/table[:#1]' --format '<encode_utf8=false>json' | jq -s '.' > tests/test_visualize/01_my_new_test.in.json
    ```
    * Где в `#1` можно задать число скачиваемых строк из YT
    * Если YT будет ругаться `Invalid UTF-8 string in JSON` — то это значит, что в какой-то колонке бинарные данные, которые в json не положишь. Поэтому нужно скачать таблицу без таких колонок, явно перечислив те колонки, которые хочется скачать через `{col1,col2,col3}` после имени таблицы и перед `[:#1]`
5. В тексте теста задать формирование nile job'ы:
    ```
    job.table('dummy').label('input').call(my_testing_method).label('output')
    ```
    таким образом задавая через label input — то, что придёт на вход методу и через label output — то, куда будет сохранён результат
6. Передать в метод `assertCorrectLocalNileRun` имена файлов с входными и ожидаемыми данными. В качестве последнего можно создать файл с пустым json массивом.
7. Для получения результата выполнения функции (и создания первичного файла с ожидаемыми данными) нужно:
    * запустить `ya make -tr --keep-temps`
    * найти созданный файл в папке теста `tests/test-results/pytest/testing_out_stuff` — у него будет постфикс `test_out.json` — и скопировать его в папку с данными теста
8. Всё, готово.
    * все тесты теперь локально можно запускать через `ya make -tr`
    * точечно запустить нужный тест `ya make -tr -F <test_name>` где test_name можно посмотреть в `ya make -tr -L`

### Известные ограничения:
* пока не поддержаны тесты на YQL Geo UDF (требует локальную геобазу и немного шаманства)
* в некоторых случаях нужно задавать схему для таблички для теста (например когда в данных null и непонятно как вывести тип данных)
  * Схему с nile типами данных можно получить из YT таблички таким скриптом: https://a.yandex-team.ru/arc/trunk/arcadia/junk/nerevar/jupyter/TASKS/yt_schema_types.ipynb


### Как написать тест на visualize_quasar_sessions
Чтобы сформировать новый тест-кейс:
1) сформировать данные для теста:
    * данные можно взять **из прокачек** (если вдруг у вас есть прокачка вашей корзинки, или откройте [произвольный релиз megamind](https://nirvana.yandex-team.ru/flow/55486b5d-1b54-4a72-882f-36c4aef3a1bf))
    * или **из продакшн логов** (можно насемплить произвольные строчки из prepared_logs_expboxes и визуализировать таким графом https://nda.ya.ru/t/p0Ajv0wl4CVWLJ (alice_parser в режиме logs))
    * Далее есть 3 кубика `ue2e Prepare Toloka Tasks`: для колоночных, ПП-шных и просто полготовки скриншотов. Соответствие их расположению [на скрине](https://jing.yandex-team.ru/files/nerevar/2022-03-04_11-07-54.png)
    * Внутри выбранного кубика переходим в `alice deep parser [with mode]` -> Вкладка "Details" в нирване -> переход в YQL запрос -> В YQL запросе нужно выбрать табличку `checkpoint-before_visualize` или `checkpoint-before_sessions` внутри вкладки "Progress": [скрин](https://jing.yandex-team.ru/files/nerevar/2022-03-04_13-25-10.png)
    * Из таблички `checkpoint-before_visualize` можно насемплить нужные вам запросы/сценарии/req_id для теста отдельный yql запросом в отдельную табличку
2) скачать данные тест-кейсов из YT — локально в json (см пункт выше про `ya tool yt read`)
3) если данные теста < 100Кб — то положить в json файл в папке test_visualize
    Большие файлы залить в sandbox по инструкции в readme.md ниже
4) для получения канонических результатов теста нужно запустить тест с параметром -Z:
    * посмотреть список тестов: `alice/analytics/operations/priemka/alice_parser $ ya make -t -L`
    * канонизировать нужный тест: `alice/analytics/operations/priemka/alice_parser $ ya make -t -F <имя_теста> -Z`

### Как хранить файлы для теста в sandbox
В аркадии можно хранить файлы, не превышающие 100 КБ, всё что больше нужно загружать в sandbox.
В sandbox можно хранить как 1) исходные данные для тестов, так и 2) результаты тестов (в случае, если проверяется определённый json на выходе)
1. Исходные данные для тестов:
    * json с тестом заливается в sandbox: `ya upload --ttl=inf --backup my_test_data.json`
    * id sandbox ресурса (sbr) указывается в ya.make в разделе DATA() с указанием папки, куда "скачивать" тест при прогоне
    * в самом тесте путь к файлу можно получить через `yatest.common.runtime.work_path`
2. Результат выполнения теста можно хранить как локальный файл в json или как ресурс в sandbox:
    * функция с тестом должна возвращать путь к файлу с данными, полученный через `yatest.common.canonical_file` + `yatest.common.output_path`
        * аргумент local=True или local=False в canonical_file определяет, хранить ли файл локально в аркадии или заливать в sandbox
    * выход функции с тестом можно канонизировать аркадией с помощью запуска `ya make -t -F <имя теста> -Z`. Появившиеся данные в папке `canondata` нужно добавить в пулл-реквест
    * коммитить результат канонизации лучше с той же машинки, где вы делали канонизацию. Если вы используете синк репозиториев своего компьютера и удаленной машинки, на которой делаете канонизацию, при коммите со своего компьютера можете встретить спецэффекты с переносами строк. Особенно в случае, если у вас на рабочем компьютере не линукс.

### Если падает тест:
   * если падает тест, который возвращает json и сравнивает его с каноническим (например `test_visualize.py::test_visualize_quasar_sessions`), то починить тест можно просто перегенерив новые канонические данные с помощью `-Z` из примера выше и добавить изменения в пулл-реквест

### Как уменьшить размер теста
* можно удалить лишние поля таким кодом https://paste.yandex-team.ru/3970923
    * только будьте внимательны, он удаляет `device_state`, в некоторых тестах он нужен