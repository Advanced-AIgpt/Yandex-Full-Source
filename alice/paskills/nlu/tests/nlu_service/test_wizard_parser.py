# coding: utf-8
from __future__ import unicode_literals
import pytest

from nlu_service import named_entity
from nlu_service.services.wizard_parser import WizardParser


@pytest.fixture
def parser():
    return WizardParser()


def test_empty_markup_returns_empty_list(parser):
    # utterance = ''
    wizard_markup = {}
    assert parser.extract_entities(wizard_markup) == []


def test_extract_integer(parser):
    # utterance = '1234'
    wizard_markup = {
        "Numbers": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 2
                },
                "Text": "1234",
                "Integer": 1234,
                "Value": 1234,
                "IsOverrided": False
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.NumberEntity(start_token=0, end_token=2, value=1234),
    ]


def test_extract_float(parser):
    # utterance = '1234.5'
    wizard_markup = {
        "Numbers": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 2
                },
                "Text": "1234",
                "Float": 1234.5,
                "Value": 1234.5,
                "IsOverrided": False
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.NumberEntity(start_token=0, end_token=2, value=1234.5),
    ]


def test_extract_full_name(parser):
    # utterance = 'иванов иван иванович'
    wizard_markup = {
        "Fio": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 3,
                },
                "LastName": "иванов",
                "Type": "fioname",
                "Patronymic": "иванович",
                "FirstName": "иван",
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(
            start_token=0,
            end_token=3,
            first_name='иван',
            last_name='иванов',
            patronymic_name='иванович',
        )
    ]


def test_extract_first_and_last_names(parser):
    # utterance = 'иван иванов'
    wizard_markup = {
        "Fio": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 2,
                },
                "LastName": "иванов",
                "Type": "finame",
                "FirstName": "иван",
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(start_token=0, end_token=2, first_name='иван', last_name='иванов', patronymic_name=None)
    ]


def test_lemmer_names_are_normalized(parser):
    # utterance = 'закажи пиццу для ивана'
    wizard_markup = {
        "Tokens": [
            {
                "EndChar": 6,
                "Text": "закажи",
                "EndByte": 12,
                "BeginByte": 0,
                "BeginChar": 0
            },
            {
                "EndChar": 12,
                "Text": "пиццу",
                "EndByte": 23,
                "BeginByte": 13,
                "BeginChar": 7
            },
            {
                "EndChar": 16,
                "Text": "для",
                "EndByte": 30,
                "BeginByte": 24,
                "BeginChar": 13
            },
            {
                "EndChar": 22,
                "Text": "ивана",
                "EndByte": 41,
                "BeginByte": 31,
                "BeginChar": 17
            }
        ],
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "заказывать",
                        "Grammems": [
                            "V sg imper 2p pf"
                        ],
                        "Language": "ru"
                    }
                ]
            },
            {
                "Tokens": {
                    "Begin": 1,
                    "End": 2
                },
                "Lemmas": [
                    {
                        "Text": "пицца",
                        "Grammems": [
                            "S acc sg f inan"
                        ],
                        "Language": "ru"
                    }
                ]
            },
            {
                "Tokens": {
                    "Begin": 2,
                    "End": 3
                },
                "Lemmas": [
                    {
                        "Text": "для",
                        "Grammems": [
                            "PR"
                        ],
                        "Language": "ru"
                    }
                ]
            },
            {
                "Tokens": {
                    "Begin": 3,
                    "End": 4
                },
                "Lemmas": [
                    {
                        "Text": "иван",
                        "Grammems": [
                            "S persn acc sg m anim",
                            "S persn gen sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            }
        ]
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(start_token=3, end_token=4, first_name='иван'),
    ]


def test_extract_first_name(parser):
    # utterance = 'иван'
    wizard_markup = {
        "Tokens": [
            {
                "EndChar": 4,
                "Text": "иван",
                "EndByte": 8,
                "BeginByte": 0,
                "BeginChar": 0
            },
        ],
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "иван",
                        "Grammems": [
                            "S persn nom sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(start_token=0, end_token=1, first_name='иван', last_name=None, patronymic_name=None),
    ]


def test_extract_first_and_patronymic_names(parser):
    # utterance = 'иван иванович'
    wizard_markup = {
        "Tokens": [
            {
                "EndChar": 4,
                "Text": "иван",
                "EndByte": 8,
                "BeginByte": 0,
                "BeginChar": 0
            },
            {
                "EndChar": 13,
                "Text": "иванович",
                "EndByte": 25,
                "BeginByte": 9,
                "BeginChar": 5
            }
        ],
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "иван",
                        "Grammems": [
                            "S persn nom sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            },
            {
                "Tokens": {
                    "Begin": 1,
                    "End": 2
                },
                "Lemmas": [
                    {
                        "Text": "иванович",
                        "Grammems": [
                            "S patrn nom sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            }
        ],

    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(start_token=0, end_token=2, first_name='иван', patronymic_name='иванович')
    ]


def test_extract_two_consequent_first_names(parser):
    # utterance = 'иван иванович'
    wizard_markup = {
        "Tokens": [
            {
                "EndChar": 4,
                "Text": "иван",
                "EndByte": 8,
                "BeginByte": 0,
                "BeginChar": 0,
            },
            {
                "EndChar": 13,
                "Text": "петр",
                "EndByte": 25,
                "BeginByte": 9,
                "BeginChar": 5,
            },
        ],
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "иван",
                        "Grammems": [
                            "S persn nom sg m anim"
                        ],
                        "Language": "ru"
                    },
                ],
            },
            {
                "Tokens": {
                    "Begin": 1,
                    "End": 2,
                },
                "Lemmas": [
                    {
                        "Text": "петр",
                        "Grammems": [
                            "S persn nom sg m anim",
                        ],
                        "Language": "ru",
                    },
                ],
            },
        ],

    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(start_token=0, end_token=1, first_name='иван'),
        named_entity.FioEntity(start_token=1, end_token=2, first_name='петр'),
    ]


def test_overlapping_fio_and_grammems(parser):
    # utterance = 'иванов иван иванович'
    wizard_markup = {
        "Tokens": [
            {
                "EndChar": 6,
                "Text": "иванов",
                "EndByte": 12,
                "BeginByte": 0,
                "BeginChar": 0
            },
            {
                "EndChar": 11,
                "Text": "иван",
                "EndByte": 21,
                "BeginByte": 13,
                "BeginChar": 7
            },
            {
                "EndChar": 20,
                "Text": "иванович",
                "EndByte": 38,
                "BeginByte": 22,
                "BeginChar": 12
            }
        ],
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "иванов",
                        "Grammems": [
                            "S famn nom sg m anim"
                        ],
                        "Language": "ru"
                    },
                    {
                        "Text": "иван",
                        "Grammems": [
                            "S persn acc pl m anim",
                            "S persn gen pl m anim"
                        ],
                        "Language": "ru"
                    },
                    {
                        "Text": "иванов",
                        "Grammems": [
                            "A acc sg plen poss m inan",
                            "A nom sg plen poss m"
                        ],
                        "Language": "ru"
                    }
                ]
            },
            {
                "Tokens": {
                    "Begin": 1,
                    "End": 2
                },
                "Lemmas": [
                    {
                        "Text": "иван",
                        "Grammems": [
                            "S persn nom sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            },
            {
                "Tokens": {
                    "Begin": 2,
                    "End": 3
                },
                "Lemmas": [
                    {
                        "Text": "иванович",
                        "Grammems": [
                            "S patrn nom sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            }
        ],
        "Fio": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 3
                },
                "LastName": "иванов",
                "Type": "fioname",
                "Patronymic": "иванович",
                "FirstName": "иван"
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.FioEntity(
            start_token=0,
            end_token=3,
            first_name='иван',
            patronymic_name='иванович',
            last_name='иванов',
        ),
    ]


def test_ignore_nondict_lemmer_words(parser):
    # utterance = 'дорасы'
    # PASKILLS-1356
    # Morph.Lemmas contains 'persn' grammem, but this grammem was generated for a foundling word
    # see https://wiki.yandex-team.ru/poiskovajaplatforma/lingvistika/lemmer/lemmer-test/ if you want to know
    # what a foundling is
    wizard_markup = {
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "дорас",
                        "Grammems": [
                            "S persn nom pl m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            }
        ],
        "Tokens": [
            {
                "EndChar": 6,
                "Text": "дорасы",
                "EndByte": 12,
                "BeginByte": 0,
                "BeginChar": 0
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == []


def test_always_ignore_obscene_words(parser):
    # that's a completely synthetic testcase, but we don't want obscene words to be marked as names
    wizard_markup = {
        "Morph": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1
                },
                "Lemmas": [
                    {
                        "Text": "блядь",
                        "Grammems": [
                            "S persn obsc nom sg m anim"
                        ],
                        "Language": "ru"
                    }
                ]
            }
        ],
        "Tokens": [
            {
                "EndChar": 5,
                "Text": "блядь",
                "EndByte": 10,
                "BeginByte": 0,
                "BeginChar": 0
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == []


def test_extract_single_geo_entity(parser):
    # utterance = 'ленина 10'
    wizard_markup = {
        "GeoAddr": [
            {
                "BestInheritedId": 213,
                "Weight": 1,
                "Fields": [
                    {
                        "Tokens": {
                            "Begin": 0,
                            "End": 1
                        },
                        "Type": "Street",
                        "Name": "ленина"
                    },
                    {
                        "Tokens": {
                            "Begin": 1,
                            "End": 2
                        },
                        "Type": "HouseNumber",
                        "Name": "10"
                    }
                ],
                "BestGeoId": -1,
                "Tokens": {
                    "Begin": 0,
                    "End": 2
                },
                "PossibleCityId": [
                ],
            },
        ]
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.GeoEntity(start_token=0, end_token=2, street='ленина', house_number='10')
    ]


def test_extract_only_first_geo_from_overlapping_entities(parser):
    # utterance = 'ленина 10'
    wizard_markup = {
        'GeoAddr': [
            {
                "BestInheritedId": 213,
                "Weight": 1,
                "Fields": [
                    {
                        "Tokens": {
                            "Begin": 0,
                            "End": 1,
                        },
                        "Type": "Street",
                        "Name": "ленина",
                    },
                    {
                        "Tokens": {
                            "Begin": 1,
                            "End": 2,
                        },
                        "Type": "HouseNumber",
                        "Name": "10",
                    }
                ],
                "BestGeoId": -1,
                "Tokens": {
                    "Begin": 0,
                    "End": 2,
                },
                "PossibleCityId": [
                ],
            },
            {
                "BestInheritedId": 213,
                "Weight": 1,
                "Fields": [
                    {
                        "Tokens": {
                            "Begin": 0,
                            "End": 1
                        },
                        "Type": "Street",
                        "Name": "ленинская"
                    },
                    {
                        "Tokens": {
                            "Begin": 1,
                            "End": 2
                        },
                        "Type": "HouseNumber",
                        "Name": "10",
                    },
                ],
                "BestGeoId": -1,
                "Tokens": {
                    "Begin": 0,
                    "End": 2,
                },
                "PossibleCityId": [
                    213,
                ],
            },
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.GeoEntity(start_token=0, end_token=2, street='ленина', house_number='10')
    ]


def test_take_first_nonempty_hypothesis(parser):
    wizard_markup = {
        'GeoAddr': [
            {
                "BestInheritedId": 213,
                "Weight": 0.9187044501,
                "Fields": [
                    {
                        "Tokens": {
                            "Begin": 3,
                            "End": 5
                        },
                        "Type": "Quarter",
                        "Name": "льва толстого"
                    }
                ],
                "BestGeoId": -1,
                "Tokens": {
                    "Begin": 2,
                    "End": 5
                },
                "PossibleCityId": [
                    15
                ]
            },
            {
                "BestInheritedId": 213,
                "Weight": 0.9187044501,
                "Fields": [
                    {
                        "Tokens": {
                            "Begin": 3,
                            "End": 5
                        },
                        "Type": "Street",
                        "Name": "льва толстого"
                    }
                ],
                "BestGeoId": -1,
                "Tokens": {
                    "Begin": 2,
                    "End": 5
                },
                "PossibleCityId": [
                ]
            },
        ]
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.GeoEntity(start_token=2, end_token=5, street='льва толстого')
    ]


def test_parse_absolute_date(parser):
    # utterance = '2 января 2018 года 13 часов 15 минут'
    wizard_markup = {
        "Date": [
            {
                "Hour": 13,
                "Min": 15,
                "Month": 1,
                "Tokens": {
                    "Begin": 0,
                    "End": 8
                },
                "Year": 2018,
                "Day": 2,
            },
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.DateTimeEntity(
            start_token=0,
            end_token=8,
            year=2018,
            month=1,
            day=2,
            hour=13,
            minute=15,
        )
    ]


def test_parse_relative_date(parser):
    # utterance = 'завтра'
    wizard_markup = {
        "Date": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 1,
                },
                "Day": 1,
                "RelativeDay": True,
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.DateTimeEntity(
            start_token=0,
            end_token=1,
            day=1,
            day_is_relative=True,
        ),
    ]


def test_parse_duration(parser):
    # utterance = 'через 15 минут'
    wizard_markup = {
        "Date": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 3,
                },
                "Duration": {
                    "Type": "FORWARD",
                    "Min": 15,
                }
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.DateTimeEntity(
            start_token=0,
            end_token=3,
            minute=15,
            minute_is_relative=True,
        )
    ]


def test_parse_mixed_datetime(parser):
    # utterance = 'завтра в 15 часов'
    wizard_markup = {
        "Date": [
            {
                "Tokens": {
                    "Begin": 0,
                    "End": 4,
                },
                "Day": 1,
                "Hour": 15,
                "RelativeDay": True,
            }
        ],
    }
    assert parser.extract_entities(wizard_markup) == [
        named_entity.DateTimeEntity(
            start_token=0,
            end_token=4,
            hour=15,
            day=1,
            day_is_relative=True,
        )
    ]
