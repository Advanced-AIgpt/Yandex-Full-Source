# Сервер-обертка над granet

## Как запускать

```
ya make --sanitize=address
./server/server
```

## Примеры запросов

Сейчас есть только тестовый скрипт, который позволяет сериализовать две грамматики (`grammar.grnt` и `grammar_2.grnt`) и прогнать для них тесты.

```
python test.py

200
{
    "result": {
        "grammar_base64": "H4sIAAAAAAAAA52PTQrCMBBGrxIG3JWCXeYqpZRBYwnmp0yiCKUguPQy7u0h0hs5QaroRjAQ5uMxMy8ZoCN0KrY7TxZje1QUtHcgYQ0FGHQdRzpwtqgzzqXsyEVGUZ1iADnkHdYiPbmsIS-TAo3eqLLHsNfGhHKrrOehlVHcy4G8j5Kr4FOnKd3yFWmar-nOYT5nMl8aboGmWBxt9dtS_aER1SJ6_5Ad2vaeXvNfb_iAC4JmHB9s0mYnVgEAAA,,",
        "true_negatives": [
            "мама не мыла раму"
        ],
        "false_negatives": [],
        "false_positives": [],
        "true_positives": [
            "мама мыла раму",
            "мама мыла раму 2"
        ]
    }
}
```

Пример запроса в визард (сериализованная грамматика передаётся в параметре `bg_granet_source_text`):

```
curl http://hamzard.yandex.net:8891/wizard?text=%D0%BC%D0%B0%D0%BC%D0%B0+%D0%BC%D1%8B%D0%BB%D0%B0+%D1%80%D0%B0%D0%BC%D1%83+2&wizclient=megamind&tld=ru&uil=ru&dbgwzr=2&format=json&wizextra=alice_preprocessing%3Dtrue%3Bbg_granet_source_text%3DH4sIAAAAAAAAA52PTQrCQAyFrzIE3JWCXc5VSilBxzI4PyUzilAKgksv494eYnojM0iVrgQDIY9Hki8ZoCN0KrYHTxZje1YUtHcgYQsFGHQdSzqxtqiznUvZkYtsRXWJAeSQd1iL9PZlDXmZFGj0TpU9hqM2JpR7ZT0PbYziXhbkfZRcBUedpvTIKdI039OTxXzNznxruAWaYmG01W9K9QdGVAvo-yEztO09feZXf66s5axmHF8SSesWUwEAAA%2C%2C&rwr=AliceAnaphoraMatcher%2CAliceAnaphoraSubstitutor%2CAliceEllipsisRewriter%2CAliceEmbeddings%2CAliceFixlist%2CAliceFrameFiller%2CAliceMicrointents%2CAliceNonsenseTagger%2CAliceNormalizer%2CAliceRequest%2CAliceSampleFeatures%2CAliceScenariosWordLstm%2CAliceSession%2CAliceTagger%2CAliceTokenEmbedder%2CAliceTolokaWordLstm%2CAliceUserEntities%2CCustomEntities%2CEntityFinder%2CFstAlbum%2CFstArtist%2CFstCalc%2CFstCurrency%2CFstDate%2CFstDatetime%2CFstDatetimeRange%2CFstFilms100_750%2CFstFilms50Filtered%2CFstFio%2CFstFloat%2CFstGeo%2CFstNum%2CFstPoiCategoryRu%2CFstSite%2CFstSoft%2CFstSwear%2CFstTime%2CFstTrack%2CFstUnitsTime%2CFstWeekdays%2CGranet%2CGranetCompiler
```

В ответе визарда будет разбор `alice.paskills.demo2`