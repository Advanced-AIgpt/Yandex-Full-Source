package help

const (
	_divCardExampleJSON = `{
    "states": [
        {
            "action": {
                "log_id": "whole_card",
                "url": "yellowskin://?primary_color=%23ffffff&secondary_color=%23000000&url=https%3A//yandex.ru/pogoda/irkutsk%3Fappsearch_header%3D1%26appsearch_ys%3D2%23d_1"
            },
            "state_id": 1,
            "blocks": [
                {
                    "type": "div-separator-block",
                    "size": "xs"
                },
                {
                    "type": "div-universal-block",
                    "title": "Сейчас в Иркутске "
                },
                {
                    "rows": [
                        {
                            "cells": [
                                {
                                    "text": " +23 °",
                                    "text_style": "numbers_l",
                                    "horizontal_alignment": "left"
                                },
                                {
                                    "image": {
                                        "ratio": 1,
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/397492/weather_80x80_e4b278a3570c48ef3de5828501f6228cef2806b5317be801ef303f7ef2b0664d.png/orig",
                                        "type": "div-image-element"
                                    },
                                    "horizontal_alignment": "left",
                                    "image_size": "xxl"
                                }
                            ],
                            "type": "row_element"
                        }
                    ],
                    "type": "div-table-block",
                    "columns": [
                        {},
                        {
                            "left_padding": "zero"
                        }
                    ]
                },
                {
                    "text": "Переменная облачность от +9 до +23 ",
                    "type": "div-universal-block"
                },
                {
                    "type": "div-separator-block",
                    "size": "xxs"
                },
                {
                    "has_delimiter": 1,
                    "type": "div-separator-block",
                    "size": "xxs"
                },
                {
                    "type": "div-separator-block",
                    "size": "s"
                },
                {
                    "rows": [
                        {
                            "cells": [
                                {
                                    "text": " Утро ",
                                    "text_style": "text_s"
                                },
                                {
                                    "text": " День ",
                                    "text_style": "text_s"
                                },
                                {
                                    "text": " Вечер ",
                                    "text_style": "text_s"
                                }
                            ],
                            "type": "row_element"
                        },
                        {
                            "cells": [
                                {
                                    "text": " +13 ",
                                    "image": {
                                        "ratio": 1,
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/787408/weather_60x60_b55184af54e7f571b2ff888c5df04585b3a969b168a4a921da10cfb3fe69437b.png/orig",
                                        "type": "div-image-element"
                                    },
                                    "image_position": "right",
                                    "image_size": "xs"
                                },
                                {
                                    "text": " +22 ",
                                    "image": {
                                        "ratio": 1,
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/787408/weather_60x60_b55184af54e7f571b2ff888c5df04585b3a969b168a4a921da10cfb3fe69437b.png/orig",
                                        "type": "div-image-element"
                                    },
                                    "image_position": "right",
                                    "image_size": "xs"
                                },
                                {
                                    "text": " +17 ",
                                    "image": {
                                        "ratio": 1,
                                        "image_url": "https://avatars.mds.yandex.net/get-bass/397492/weather_60x60_3d8421fa4ebf65116b130d17eac1a61d4f91485e9c0703aa53a6925d6e6876d0.png/orig",
                                        "type": "div-image-element"
                                    },
                                    "image_position": "right",
                                    "image_size": "xs"
                                }
                            ],
                            "type": "row_element"
                        }
                    ],
                    "type": "div-table-block",
                    "columns": [
                        {
                            "right_padding": "xxl"
                        },
                        {
                            "right_padding": "xxl"
                        },
                        {
                            "weight": 0
                        }
                    ]
                },
                {
                    "type": "div-separator-block",
                    "size": "s"
                }
            ]
        }
    ],
    "background": [
        {
            "color": "#FFFFFF",
            "type": "div-solid-background"
        }
    ]
}`
)
