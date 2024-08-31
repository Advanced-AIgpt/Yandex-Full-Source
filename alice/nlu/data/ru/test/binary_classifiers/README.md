# Binary Classifiers

На данный момент поддерживаются только модели beggins_models.

## Medium tests

1. Добавьте модель в `alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc`
2. Добавьте тест кейс `alice/nlu/data/ru/test/binary_classifiers/medium/ut/test.py` следующего вида
   ```json
    {
        "name": "qr_code", # unique
        "model": "AliceBegginsFixlistQRCode", # from data.inc
        "threshold": 2.5590160000206197, # your own
        "pool": ALICE6V3,
    }
    ```
3. Запустите канонизацию результатов `YQL_TOKEN=<your-yql-token> ya make -j32 -ttt --yt-store --test-param canonize=true`
4. Добавьте полученный файл из `alice/nlu/data/ru/test/binary_classifiers/medium` в коммит.

Для тестирования запустите команду `YQL_TOKEN=<your-yql-token> ya make -j32 -ttt --yt-store`

# Small tests

1. Добавьте модель в `alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc`
2. Добавьте тест кейс `alice/nlu/data/ru/test/binary_classifiers/small/ut/test.py` следующего вида
   ```json
    {
        "name": "qr_code", # unique
        "model": "AliceBegginsFixlistQRCode", # from data.inc
        "threshold": 2.5590160000206197, # your own
    }
    ```
3. Добавить файлы `<name>.positives.txt` и `<name>.negatives.txt` в папку `alice/nlu/data/ru/test/binary_classifiers/small/target`

Для тестирования запустите команду `YQL_TOKEN=<your-yql-token> ya make -j32 -ttt --yt-store`
