# coding: utf-8

from __future__ import unicode_literals

from collections import OrderedDict
from vins_core.dm.request import AppInfo


TEST_FRAMEWORK_YAML = """
labels:
  regexp_example: 'a(b|c)'

freeze_time: common_time
geo:
  lon: 37.1234
  lat: 55.4321
experiments: [exp1]
app_info:
  app_id: 'telegram'
  app_version: '0'
  os_version: '0'
  platform: 'telegram'
device_state:
  sound_level: 1
bass:
  form:
    common_slot:
        value: common_slot_value
case1:
  bass:
    form:
      complex_slot:
        value:
            f1: v1
      optionality_changed_slot:
        optional: false
    blocks:
    - type: suggest
      data:
        code: 1
    - type: command
      command_type: x
      data: data1
  dialog:
    utterance1:
      experiments: [exp2]
      bass:
        form:
          slot1:
            value: value1
          complex_slot:
            value:
                f2: v2
        blocks:
          - type: suggest
            data:
              code: 2
          - type: command
            command_type: y
            data: data2
      vins_form:
        name: test_name
        slots:
            simple_slot:
                value: value0
            dict_slot:
                value:
                    key1: value1
                    key2: value2
            list_dict_slot:
                value:
                    - dict1key1: dict1value1
                      dict1key2: dict1value2
                    - dict2key1: dict2value1
                      dict2key2: dict2value2
      voice: voice1
      text: 'text1{regexp_example}'
      suggests:
      - caption: suggest_caption_1
        utterance: suggest_utterance_1
      - caption: suggest_caption_2
        utterance: suggest_caption_2
      - caption: suggest_caption_3
        user_utterance: suggest_user_utterance_1
      button_actions:
      - name: action_name_1
        title: action_title_1
        payload:
          uri: action_uri_1
      - name: action_name_2
        title: action_title_2
        payload:
          uri: action_uri_2
      directives:
      - name: x
        payload: data1
      - name: y
        payload: data2
time_test_group:
  experiments:
    exp3: '1'
    exp1: null
  bass:
  case2:
    freeze_time: new_time
    app_info: null
    bass:
      form:
        common_slot: null
    dialog:
      utterance2:
        freeze_time: null
        voice: voice2
        text: text2
        directives:
          exact_match: true
          data: []
        meta:
          - type: meta1
          - type: meta2
  case3:
    geo: null
    dialog:
      utterance3:
        voice: voice3
        text: text3
        suggests:
          exact_match: true
          data:
            - caption: suggest_caption_1
              utterance: suggest_utterance_1
        meta:
          exact_match: true
          data:
            - type: 'meta1'
case4:
  dialog:
  - request:
      utterance4
    response:
      text: text4
  - request:
      text: utterance5
      type: text_input
      payload:
        field: data
    response:
      text: text5
case5:
  app_info:
    ignore:
      - pa_ios
  dialog:
    utterance6: text6
  bass:
      form:
        common_slot: null
case6:
  app_info:
    ignore:
      - pa_ios
      - pa_android
      - navigator_android
  dialog:
    utterance7: text7
  bass:
      form:
        common_slot: null
"""

TELEGRAM_APP_INFO = AppInfo(
    app_id='telegram',
    app_version='0',
    os_version='0',
    platform='telegram',
    device_manufacturer=''
)

APP_INFOS = OrderedDict([
    ('pa_android', {
        'app_id': 'ru.yandex.searchplugin.vins_test',
        'app_version': '10',
        'os_version': '8.1.0',
        'platform': 'android'

    }),
    ('pa_ios', {
        'app_id': 'ru.yandex.searchplugin.vins_test',
        'app_version': '10',
        'os_version': '11.0',
        'platform': 'iphone'
    }),
    ('navigator_android', {
        'app_id': 'ru.yandex.mobile.navigator.vins_test',
        'app_version': '10',
        'os_version': '8.1.0',
        'platform': 'android'
    })
])

APP_INFOS_AS_OBJECTS = {
    'pa_android': AppInfo(
        app_id='ru.yandex.searchplugin.vins_test',
        app_version='10',
        os_version='8.1.0',
        platform='android',
        device_manufacturer=''
    ),
    'pa_ios': AppInfo(
        app_id='ru.yandex.searchplugin.vins_test',
        app_version='10',
        os_version='11.0',
        platform='iphone',
        device_manufacturer=''
    ),
    'navigator_android': AppInfo(
        app_id='ru.yandex.mobile.navigator.vins_test',
        app_version='10',
        os_version='8.1.0',
        platform='android',
        device_manufacturer=''
    )

}

DEVICE_STATE = {
    'sound_level': 1,
}

CORRECT_TEST_DATA_WITH_APP_INFOS = [
    (
        'test::case1',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        TELEGRAM_APP_INFO,
        DEVICE_STATE,
        [
            [
                # Request utterance
                'utterance1',
                # Text response
                '^text1(?:a(b|c))$',
                # Voice response
                '^voice1$',
                # Meta
                None,
                # exact_meta_match
                False,
                # Suggests
                [
                    {
                        'caption': '^suggest\\_caption\\_1$',
                        'utterance': '^suggest\\_utterance\\_1$'
                    },
                    {
                        'caption': '^suggest\\_caption\\_2$',
                        'utterance': '^suggest\\_caption\\_2$'
                    },
                    {
                        'caption': '^suggest\\_caption\\_3$',
                        'user_utterance': '^suggest\\_user\\_utterance\\_1$'
                    }
                ],
                # exact_suggests_match
                False,
                # Actions
                [
                    {
                        'name': 'action_name_1',
                        'title': 'action_title_1',
                        'payload': {
                            'uri': 'action_uri_1'
                        }
                    },
                    {
                        'name': 'action_name_2',
                        'title': 'action_title_2',
                        'payload': {
                            'uri': 'action_uri_2'
                        }
                    }
                ],
                # Directives
                [
                    {
                        'name': 'x',
                        'payload': 'data1'
                    },
                    {
                        'name': 'y',
                        'payload': 'data2'
                    }
                ],
                # exact_directives_match
                False,
                # Vins form
                {
                    'name': 'test_name',
                    'slots': {
                        'simple_slot': {
                            'value': 'value0',
                        },
                        'dict_slot': {
                            'value': {
                                'key1': 'value1',
                                'key2': 'value2'
                            }
                        },
                        'list_dict_slot': {
                            'value': [
                                {
                                    'dict1key1': 'dict1value1',
                                    'dict1key2': 'dict1value2'
                                },
                                {
                                    'dict2key1': 'dict2value1',
                                    'dict2key2': 'dict2value2'
                                }
                            ]
                        }
                    }
                },
                # Experiments
                {'exp1': '1', 'exp2': '1'},
                # Joint bass response
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value',
                        },
                        'optionality_changed_slot': {
                            'optional': False
                        },
                        'slot1': {
                            'value': 'value1',
                        },
                        'complex_slot': {
                            'value': {
                                'f1': 'v1',
                                'f2': 'v2'
                            }
                        }
                    },
                    'blocks': [
                        {
                            'type': 'suggest',
                            'data': {
                                'code': 1
                            }
                        },
                        {
                            'type': 'command',
                            'command_type': 'x',
                            'data': 'data1'
                        },
                        {
                            'type': 'suggest',
                            'data': {
                                'code': 2
                            }
                        },
                        {
                            'type': 'command',
                            'command_type': 'y',
                            'data': 'data2'
                        }
                    ]
                }
            ]
        ]
    ),
    (
        'test::time_test_group::case2',
        'new_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        APP_INFOS_AS_OBJECTS['pa_android'],
        DEVICE_STATE,
        [
            [
                'utterance2',
                '^text2$',
                '^voice2$',
                [
                    {
                        'type': 'meta1'
                    },
                    {
                        'type': 'meta2'
                    }
                ],
                False,
                None,
                False,
                None,
                [],
                True,
                None,
                {'exp1': None, 'exp3': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::time_test_group::case2',
        'new_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        APP_INFOS_AS_OBJECTS['pa_ios'],
        DEVICE_STATE,
        [
            [
                'utterance2',
                '^text2$',
                '^voice2$',
                [
                    {
                        'type': 'meta1'
                    },
                    {
                        'type': 'meta2'
                    }
                ],
                False,
                None,
                False,
                None,
                [],
                True,
                None,
                {'exp1': None, 'exp3': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::time_test_group::case2',
        'new_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        APP_INFOS_AS_OBJECTS['navigator_android'],
        DEVICE_STATE,
        [
            [
                'utterance2',
                '^text2$',
                '^voice2$',
                [
                    {
                        'type': 'meta1'
                    },
                    {
                        'type': 'meta2'
                    }
                ],
                False,
                None,
                False,
                None,
                [],
                True,
                None,
                {'exp1': None, 'exp3': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::time_test_group::case3',
        'common_time',
        None,
        TELEGRAM_APP_INFO,
        DEVICE_STATE,
        [
            [
                'utterance3',
                '^text3$',
                '^voice3$',
                [
                    {
                        'type': 'meta1'
                    }
                ],
                True,
                [
                    {
                        'caption': '^suggest\\_caption\\_1$',
                        'utterance': '^suggest\\_utterance\\_1$'
                    }
                ],
                True,
                None,
                None,
                False,
                None,
                {'exp1': None, 'exp3': '1'},
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value'
                        }
                    },
                    'blocks': []
                }
            ]
        ]
    ),
    (
        'test::case4',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        TELEGRAM_APP_INFO,
        DEVICE_STATE,
        [
            [
                'utterance4',
                '^text4$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value'
                        }
                    },
                    'blocks': []
                }
            ],
            [
                {
                    'text': 'utterance5',
                    'type': 'text_input',
                    'payload': {'field': 'data'}
                },
                '^text5$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value'
                        }
                    },
                    'blocks': []
                }
            ]
        ]
    ),
    (
        'test::case5',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        APP_INFOS_AS_OBJECTS['pa_android'],
        DEVICE_STATE,
        [
            [
                'utterance6',
                '^text6$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::case5',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        APP_INFOS_AS_OBJECTS['navigator_android'],
        DEVICE_STATE,
        [
            [
                'utterance6',
                '^text6$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::case6',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        None,
        DEVICE_STATE,
        [
            [
                'utterance7',
                '^text7$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    )
]
CORRECT_TEST_DATA_WITHOUT_APP_INFOS = [
    (
        'test::case1',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        TELEGRAM_APP_INFO,
        DEVICE_STATE,
        [
            [
                # Request utterance
                'utterance1',
                # Text response
                '^text1(?:a(b|c))$',
                # Voice response
                '^voice1$',
                # Meta
                None,
                # exact_meta_match
                False,
                # Suggests
                [
                    {
                        'caption': '^suggest\\_caption\\_1$',
                        'utterance': '^suggest\\_utterance\\_1$'
                    },
                    {
                        'caption': '^suggest\\_caption\\_2$',
                        'utterance': '^suggest\\_caption\\_2$'
                    },
                    {
                        'caption': '^suggest\\_caption\\_3$',
                        'user_utterance': '^suggest\\_user\\_utterance\\_1$'
                    }
                ],
                # exact_suggests_match
                False,
                # Actions
                [
                    {
                        'name': 'action_name_1',
                        'title': 'action_title_1',
                        'payload': {
                            'uri': 'action_uri_1'
                        }
                    },
                    {
                        'name': 'action_name_2',
                        'title': 'action_title_2',
                        'payload': {
                            'uri': 'action_uri_2'
                        }
                    }
                ],
                # Directives
                [
                    {
                        'name': 'x',
                        'payload': 'data1'
                    },
                    {
                        'name': 'y',
                        'payload': 'data2'
                    }
                ],
                # exact_directives_match
                False,
                # Vins form
                {
                    'name': 'test_name',
                    'slots': {
                        'simple_slot': {
                            'value': 'value0',
                        },
                        'dict_slot': {
                            'value': {
                                'key1': 'value1',
                                'key2': 'value2'
                            }
                        },
                        'list_dict_slot': {
                            'value': [
                                {
                                    'dict1key1': 'dict1value1',
                                    'dict1key2': 'dict1value2'
                                },
                                {
                                    'dict2key1': 'dict2value1',
                                    'dict2key2': 'dict2value2'
                                }
                            ]
                        }
                    }
                },
                # Experiments
                {'exp1': '1', 'exp2': '1'},
                # Joint bass response
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value',
                        },
                        'optionality_changed_slot': {
                            'optional': False
                        },
                        'slot1': {
                            'value': 'value1',
                        },
                        'complex_slot': {
                            'value': {
                                'f1': 'v1',
                                'f2': 'v2'
                            }
                        }
                    },
                    'blocks': [
                        {
                            'type': 'suggest',
                            'data': {
                                'code': 1
                            }
                        },
                        {
                            'type': 'command',
                            'command_type': 'x',
                            'data': 'data1'
                        },
                        {
                            'type': 'suggest',
                            'data': {
                                'code': 2
                            }
                        },
                        {
                            'type': 'command',
                            'command_type': 'y',
                            'data': 'data2'
                        }
                    ]
                }
            ]
        ]
    ),
    (
        'test::time_test_group::case2',
        'new_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        None,
        DEVICE_STATE,
        [
            [
                'utterance2',
                '^text2$',
                '^voice2$',
                [
                    {
                        'type': 'meta1'
                    },
                    {
                        'type': 'meta2'
                    }
                ],
                False,
                None,
                False,
                None,
                [],
                True,
                None,
                {'exp1': None, 'exp3': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::time_test_group::case3',
        'common_time',
        None,
        TELEGRAM_APP_INFO,
        DEVICE_STATE,
        [
            [
                'utterance3',
                '^text3$',
                '^voice3$',
                [
                    {
                        'type': 'meta1'
                    }
                ],
                True,
                [
                    {
                        'caption': '^suggest\\_caption\\_1$',
                        'utterance': '^suggest\\_utterance\\_1$'
                    }
                ],
                True,
                None,
                None,
                False,
                None,
                {'exp1': None, 'exp3': '1'},
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value'
                        }
                    },
                    'blocks': []
                }
            ]
        ]
    ),
    (
        'test::case4',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        TELEGRAM_APP_INFO,
        DEVICE_STATE,
        [
            [
                'utterance4',
                '^text4$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value'
                        }
                    },
                    'blocks': []
                }
            ],
            [
                {
                    'text': 'utterance5',
                    'type': 'text_input',
                    'payload': {'field': 'data'}
                },
                '^text5$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {
                        'common_slot': {
                            'value': 'common_slot_value'
                        }
                    },
                    'blocks': []
                }
            ]
        ]
    ),
    (
        'test::case5',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        None,
        DEVICE_STATE,
        [
            [
                'utterance6',
                '^text6$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    ),
    (
        'test::case6',
        'common_time',
        {
            'lat': 55.4321,
            'lon': 37.1234
        },
        None,
        DEVICE_STATE,
        [
            [
                'utterance7',
                '^text7$',
                None,
                None,
                False,
                None,
                False,
                None,
                None,
                False,
                None,
                {'exp1': '1'},
                {
                    'form_name': None,
                    'form': {},
                    'blocks': []
                },
            ]
        ]
    )
]
