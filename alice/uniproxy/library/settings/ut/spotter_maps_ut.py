from alice.uniproxy.library.settings.spotter_maps import read_maps, SpotterMaps


def test_read_maps():
    entries = [i for i in read_maps([
        {"a": 10, "b": 20, "c": 30},
        {"a": 11, "b": 20, "list": [
            {"c": 31},
            {"b": 21, "c": 32},
            {"b": 23, "list": [
                {"c": 33},
                {"c": 34}
            ]},
        ]},
        {"a": 12, "list": [
            {"b": 20, "c": 30},
            {"b": 21, "list": [
                {"c": 31},
                {"a": 13, "c": 32},
                {"a": 14, "c": 33},
            ]}
        ]}
    ])]

    assert entries.pop(0) == {"a": 10, "b": 20, "c": 30}
    assert entries.pop(0) == {"a": 11, "b": 20, "c": 31}
    assert entries.pop(0) == {"a": 11, "b": 21, "c": 32}
    assert entries.pop(0) == {"a": 11, "b": 23, "c": 33}
    assert entries.pop(0) == {"a": 11, "b": 23, "c": 34}
    assert entries.pop(0) == {"a": 12, "b": 20, "c": 30}
    assert entries.pop(0) == {"a": 12, "b": 21, "c": 31}
    assert entries.pop(0) == {"a": 13, "b": 21, "c": 32}
    assert entries.pop(0) == {"a": 14, "b": 21, "c": 33}


def test_spotter_maps():
    maps = SpotterMaps([
        {
            "lang": "ru-RU",
            "list": [
                {
                    "from": "topic-first",
                    "list": [
                        {"phrase": "алис", "to": "dst-topic-first"},
                        {"phrase": "default", "to": "dst-topic-default"}
                    ]
                },

                {
                    "from": "topic*",
                    "phrase": "алис",
                    "to": "dst-topic*"
                },

                {
                    "from": "*any*",
                    "list": [
                        {"phrase": "яндекс", "to": "dst-*any*"},
                        {"to": "dst-*any*-default"}
                    ]
                }
            ]
        },

        {
            "lang": "en-*",
            "from": "topic-en*",
            "phrase": "элис",
            "to": "dst-topic-en"
        },

        {
            "lang": "*-*",
            "from": "*",
            "phrase": "default",
            "to": "dst-*-default"
        }
    ])

    assert maps.get("ru-RU", "topic-first", "алисонька") == "dst-topic-first"
    assert maps.get("ru-RU", "topic-first", "иоанн") == "dst-topic-default"

    assert maps.get("ru-RU", "topic-second", "алиса") == "dst-topic*"
    assert maps.get("ru-RU", "topic-second", "иоанн") == "topic-second"  # no topic for default phrase

    assert maps.get("ru-RU", "a-any-a", "яндекс послушай") == "dst-*any*"
    assert maps.get("ru-RU", "b-any-b", "иоанн послушай") == "dst-*any*-default"

    assert maps.get("en-EN", "topic-en-first", "элис") == "dst-topic-en"
    assert maps.get("en-TR", "topic-en-second", "элис") == "dst-topic-en"
    assert maps.get("en-KZ", "topic-kz-any", "элис") == "topic-kz-any"  # topic-kz doesn't match topic-en*

    assert maps.get("ru-EN", "bla-bla-bla", "иоанн") == "dst-*-default"
    assert maps.get("kz-KZ", "tro-lo-lo", "иоанн") == "dst-*-default"
