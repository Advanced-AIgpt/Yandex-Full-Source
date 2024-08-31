import base64
import json
import urllib

import pytest
from alice.hollywood.library.scenarios.messenger_call.proto.call_payload_pb2 import TCallPayload
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice, callback, server_action
from alice.megamind.protos.common.permissions_pb2 import TPermissions
from google.protobuf import json_format

import predefined_contacts


OPEN_ADDRESS_BOOK = 'open_address_book'

ORDINALS = [
    'Первый',
    'Второй',
    'Третий',
    'Четвёртый',
    'Пятый',
    'Шестой',
    'Седьмой',
    'Восьмой',
    'Девятый',
    'Десятый',
]

CONTACT_NOT_FOUND_NLG = {
    'Ой, этого контакта нет или я не расслышала.',
    'Этого контакта нет или я не расслышала.',
}

'''
Дом:
    Спальня:
        1 Никита yandexstation
        2 Стас yandexmini
    Кухня:
        3 Гена yandexstation
    Гостиная:
        4 Турбо yandexstation
    Прихожая:
        5 солнышко yandexmodule

Дача:
    Коридор:
        6 Грабли yandexmini
        7 Лопата yandexstation
    Спальня:
        8 Цветок yandexstation
    Гостиная:
        9 Кошка yandexmodule
'''
COMMON_IOT_USER_INFO = '''
{
    "devices": [
        {
            "id": "feedface-e8a2-4439-b2e7-000000000001.yandexstation",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000001.yandexstation",
                "platform": "yandexstation"
            },
            "name": "Никита",
            "room_id": "0",
            "household_id": "household-1"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000002.yandexmini",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000002.yandexmini",
                "platform": "yandexmini"
            },
            "name": "Стас",
            "room_id": "0",
            "household_id": "household-1"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000003.yandexstation",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000003.yandexstation",
                "platform": "yandexstation"
            },
            "name": "Гена",
            "room_id": "1",
            "household_id": "household-1"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000004.yandexstation",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000004.yandexstation",
                "platform": "yandexstation"
            },
            "name": "Турбо",
            "room_id": "2",
            "household_id": "household-1"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000005.yandexmodule",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000005.yandexmodule",
                "platform": "yandexmodule"
            },
            "name": "солнышко",
            "room_id": "3",
            "household_id": "household-1"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000006.yandexmini",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000006.yandexmini",
                "platform": "yandexmini"
            },
            "name": "Грабли",
            "room_id": "dacha-0",
            "household_id": "household-2"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000007.yandexstation",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000007.yandexstation",
                "platform": "yandexstation"
            },
            "name": "Лопата",
            "room_id": "dacha-0",
            "household_id": "household-2"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000008.yandexstation",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000008.yandexstation",
                "platform": "yandexstation"
            },
            "name": "Цветок",
            "room_id": "dacha-1",
            "household_id": "household-2"
        },
        {
            "id": "feedface-e8a2-4439-b2e7-000000000009.yandexmodule",
            "group_ids": [
                "0cbc849b-4d29-4c13-844d-3968aa7475f3"
            ],
            "quasar_info": {
                "device_id": "feedface-e8a2-4439-b2e7-000000000009.yandexmodule",
                "platform": "yandexmodule"
            },
            "name": "Кошка",
            "room_id": "dacha-2",
            "household_id": "household-2"
        }
    ],
    "rooms": [
        {
            "id": "0",
            "name": "Спальня",
            "household_id": "household-1"
        },
        {
            "id": "1",
            "name": "Кухня",
            "household_id": "household-1"
        },
        {
            "id": "2",
            "name": "Гостиная",
            "household_id": "household-1"
        },
        {
            "id": "3",
            "name": "Прихожая",
            "household_id": "household-1"
        },
        {
            "id": "dacha-0",
            "name": "Коридор",
            "household_id": "household-2"
        },
        {
            "id": "dacha-1",
            "name": "Спальня",
            "household_id": "household-2"
        },
        {
            "id": "dacha-2",
            "name": "Гостиная",
            "household_id": "household-2"
        }
    ],
    "households": [
        {
            "id": "household-1",
            "name": "Дом"
        },
        {
            "id": "household-2",
            "name": "Дача"
        }
    ]
}
'''

STATION1_ID = 'feedface-e8a2-4439-b2e7-000000000001.yandexstation'
STATION2_ID = 'feedface-e8a2-4439-b2e7-000000000002.yandexmini'
STATION3_ID = 'feedface-e8a2-4439-b2e7-000000000003.yandexstation'
STATION4_ID = 'feedface-e8a2-4439-b2e7-000000000004.yandexstation'
STATION6_ID = 'feedface-e8a2-4439-b2e7-000000000006.yandexmini'
STATION7_ID = 'feedface-e8a2-4439-b2e7-000000000007.yandexstation'
STATION8_ID = 'feedface-e8a2-4439-b2e7-000000000008.yandexstation'

CHOOSE_DEVICE = 'choose_device'
CALL_TO = 'call_to'


GET_CALLER_NAME_SEMANTIC_FRAME = {
    "typed_semantic_frame": {
        "get_caller_name": {
            "caller_device_id": {
                "string_value": "",
            },
        },
    },
    "utterance": "",
    "analytics": {
        "product_scenario": "MessengerCall",
        "origin": "SmartSpeaker",
        "purpose": "say_caller_name",
    }
}


def _proto_to_dict(proto_message):
    json_str = json_format.MessageToJson(proto_message)
    return json.loads(json_str)


def _parse_callback(callback_pb):
    result = _proto_to_dict(callback_pb)
    result.setdefault('payload', {}).update({
        '@scenario_name': 'MessengerCall'
    })
    return callback(**result)


def _make_phone_call_nlg(name):
    return {
        f'{name}, набираю.',
        f'{name}, звоню.',
        f'{name}, уже набираю.',
        f'{name}, сейчас позвоним.',
    }


def _make_device_call_nlg(name):
    return {
        f'{name}, набираю.',
        f'{name}, звоню.',
        f'{name}, уже набираю.',
        f'{name}, сейчас позвоним.',
        f'Набираю {name}.',
        f'Звоню {name}.',
    }


def _device_call_url(device_name, device_id):
    url = 'messenger://call/create/private'
    url += '?device_id=' + device_id
    url += '&title=' + urllib.parse.quote_plus(device_name.encode('utf-8'))
    return url


def _check_openuri_directive(directives, device_name, device_id):
    assert len(directives) == 1
    expected_url = _device_call_url(device_name, device_id)
    assert directives[0].OpenUriDirective is not None
    assert directives[0].OpenUriDirective.Uri == expected_url


def _check_messenger_call_directive(directives, device_id):
    assert len(directives) == 1
    assert directives[0].MessengerCallDirective.CallToRecipient.Recipient.DeviceId == device_id


def _check_push_directive(directives):
    assert len(directives) == 1
    assert directives[0].SendPushDirective is not None
    assert directives[0].SendPushDirective.PushId == 'alice.device_to_device_call'
    settings = directives[0].SendPushDirective.Settings
    assert settings.Title == 'Выбрать устройство для звонка'
    assert settings.Text == 'Задайте названия устройствам или комнатам'
    assert settings.Link == 'opensettings://?screen=quasar'


def _check_open_quasar_screen_directive(directives):
    assert len(directives) == 1
    assert directives[0].OpenUriDirective.Uri == 'opensettings://?screen=quasar'


def _make_choose_phone_nlg(name):
    return f'{name}. Выберите, на какой номер звонить.'


def _test_repeated_query(alice, replies, query="позвони егору"):
    for reply in replies:
        response = alice(voice(query))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == reply

        _check_open_address_book_action_present(response)


def _check_contacts_text_card(response, expected_contacts):
    cards = response.run_response.ResponseBody.Layout.Cards
    assert len(cards) == 2
    assert cards[0].Text == 'Выберите, кому звонить.'
    assert cards[1].Text == '\n'.join(expected_contacts)


def _check_contacts_div_card(response, expected_contacts, has_all_contacts=True):
    div_card = response.run_response.ResponseBody.Layout.Cards[0].Div2CardExtended.Body['states'][0]['div']

    if has_all_contacts:
        assert div_card['all_contacts_action']['log_id']
        assert div_card['all_contacts_action']['url'] == '@@mm_deeplink#open_address_book'
        assert div_card['all_contacts_text'] == 'Все контакты'
    else:
        assert 'all_contacts_action' not in div_card
        assert 'all_contacts_text' not in div_card

    actual_contacts = div_card['contacts']
    assert len(actual_contacts) == len(expected_contacts)

    actual_names = [(contact['profile_name'], contact['action']['url']) for contact in actual_contacts].sort()
    expected_names = [(contact[0], f'@@mm_deeplink#choose_contact_div_{contact[1]}') for contact in expected_contacts].sort()

    assert actual_names == expected_names

    for actual_contact in actual_contacts:
        assert actual_contact['profile_image'] == 'https://static-alice.s3.yandex.net/scenarios/call/default_avatar.png'
        assert actual_contact['action']['log_id']


def _check_contacts_card(response, expected_contacts):
    cards = response.run_response.ResponseBody.Layout.Cards
    if cards[0].HasField('Div2CardExtended'):
        _check_contacts_div_card(response, expected_contacts)
    else:
        _check_contacts_text_card(
            response,
            [
                name
                for name, _ in expected_contacts
            ]
        )


def _check_phone_text_card(response, expected_profile_name, expected_phones):
    cards = response.run_response.ResponseBody.Layout.Cards
    assert len(cards) == 2
    assert cards[0].Text == _make_choose_phone_nlg(expected_profile_name)
    assert cards[1].Text == '\n'.join(expected_phones)


def _check_phone_div_card(response, expected_profile_name, expected_phones):
    div_card = response.run_response.ResponseBody.Layout.Cards[0].Div2CardExtended.Body['states'][0]['div']

    assert div_card['profile_name'] == expected_profile_name

    actual_phones = div_card['contacts']
    assert len(actual_phones) == len(expected_phones)

    for i, (actual_phone, expected_phone) in enumerate(zip(actual_phones, expected_phones)):
        assert actual_phone['phone_number'] == expected_phone
        assert actual_phone['action']['log_id']
        assert actual_phone['action']['url'] == f'@@mm_deeplink#phone_call_{i}'


def _check_phone_card(response, expected_profile_name, expected_phones):
    cards = response.run_response.ResponseBody.Layout.Cards
    if cards[0].HasField('Div2CardExtended'):
        _check_phone_div_card(response, expected_profile_name, expected_phones)
    else:
        _check_phone_text_card(response, expected_profile_name, expected_phones)


def _check_open_address_book_action(frame_action):
    assert len(frame_action.Directives.List) == 1
    assert frame_action.Directives.List[0].OpenUriDirective.Uri == 'contacts://address_book'

    assert frame_action.NluHint.FrameName == 'alice.phone_call.open_address_book'


def _check_open_address_book_action_present(response):
    frame_actions = response.run_response.ResponseBody.FrameActions
    assert OPEN_ADDRESS_BOOK in frame_actions
    _check_open_address_book_action(frame_actions[OPEN_ADDRESS_BOOK])


def _check_open_address_book_action_absent(response):
    frame_actions = response.run_response.ResponseBody.FrameActions
    assert OPEN_ADDRESS_BOOK not in frame_actions


def _check_matched_contacts_analytics(response, expected_lookup_keys):
    analytics = response.run_response.ResponseBody.AnalyticsInfo
    assert analytics.ProductScenarioName == 'call'
    assert analytics.Intent == 'phone_call'
    matched_contacts = analytics.Objects[0]
    assert matched_contacts.Id == 'phone_contacts'
    assert matched_contacts.Name == 'matched phone contacts'
    assert matched_contacts.HumanReadable == 'Найденные телефонные контакты'

    actual_lookup_keys = [
        contact.LookupKey
        for contact in matched_contacts.MatchedContacts.Contacts
    ]
    assert actual_lookup_keys == expected_lookup_keys


def _check_phone_book_analytics(response, expected_lookup_keys, expected_phones):
    analytics = response.run_response.ResponseBody.AnalyticsInfo
    assert analytics.ProductScenarioName == 'call'
    assert analytics.Intent == 'phone_call'
    phone_book = analytics.Objects[1]
    assert phone_book.Id == 'phone_book'
    assert phone_book.Name == 'phone book'
    assert phone_book.HumanReadable == 'Контактная книга'

    actual_lookup_keys = [
        contact.LookupKey
        for contact in phone_book.PhoneBook.Contacts
    ]
    actual_phones = [
        phone.Phone
        for phone in phone_book.PhoneBook.Phones
    ]
    assert actual_lookup_keys == expected_lookup_keys
    assert actual_phones == expected_phones


@pytest.fixture(scope="module")
def enabled_scenarios():
    return ['messenger_call']


@pytest.mark.scenario(name='MessengerCall', handle='messenger_call')
@pytest.mark.experiments(
    'hw_enable_phone_calls',
    'mm_enable_begemot_contacts',
)
class TestPhoneCallBase(object):
    pass


@pytest.mark.scenario(name='MessengerCall', handle='messenger_call')
@pytest.mark.experiments(
    'enable_outgoing_device_calls',
)
@pytest.mark.iot_user_info(COMMON_IOT_USER_INFO)
class TestDeviceCallBase(object):
    pass


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.contacts(predefined_contacts.TWO_EGORS)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
class TestPhoneCall(TestPhoneCallBase):

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_phone_call(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'
        assert len(layout.SuggestButtons) == 3

        _check_contacts_card(
            response,
            [
                ('Егор Мурманск Пархоменко', 'b28f7f625a8c6fa6ff0c9'),
                ('Егор Сосед По Участку', 'eab9dad60848f4f41a0e6'),
            ],
        )

        _check_open_address_book_action_present(response)

        choose_contact_callback = _parse_callback(
            response.run_response.ResponseBody.FrameActions['choose_contact_b28f7f625a8c6fa6ff0c9'].Callback
        )

        response = alice(choose_contact_callback)
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == _make_choose_phone_nlg('Егор Мурманск Пархоменко')
        assert len(layout.SuggestButtons) == 3

        _check_phone_card(
            response,
            'Егор Мурманск Пархоменко',
            [
                '+7(469)483-19-64',
                '+7(643)970 47 39',
            ],
        )

        _check_open_address_book_action_present(response)

        assert len(response.run_response.ResponseBody.FrameActions) == 6
        for name, frame_action in response.run_response.ResponseBody.FrameActions.items():
            if name == OPEN_ADDRESS_BOOK:
                continue

            if name == '3':
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == 'Первый'
                continue

            if name == '4':
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == 'Второй'
                continue

            if name == '5':
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == 'Открой контакты'
                continue

            choose_phone_callback = _parse_callback(frame_action.Callback)

            response = alice(choose_phone_callback)
            assert response.scenario_stages() == {'run'}

            layout = response.run_response.ResponseBody.Layout
            assert layout.OutputSpeech in _make_phone_call_nlg('Егор Мурманск Пархоменко')
            assert layout.Directives[0].OpenUriDirective.Uri in {
                'tel:+74694831964',
                'tel:+76439704739',
            }

            _check_open_address_book_action_present(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_call_on_single_match(self, alice):
        response = alice(voice('позвони егору соседу'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_phone_call_nlg('Егор Сосед По Участку')
        assert layout.Directives[0].OpenUriDirective.Uri.startswith('tel:')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.unsupported_features('open_address_book')
    def test_no_all_contacts_action(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_contacts_div_card(
            response,
            [
                ('Егор Мурманск Пархоменко', 'b28f7f625a8c6fa6ff0c9'),
                ('Егор Сосед По Участку', 'eab9dad60848f4f41a0e6'),
            ],
            has_all_contacts=False,
        )

        _check_open_address_book_action_absent(response)

        response = alice(voice('первый'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == _make_choose_phone_nlg('Егор Мурманск Пархоменко')
        assert len(layout.SuggestButtons) == 2

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.experiments('hw_enable_phone_book_analytics')
    def test_analytics_info(self, alice):
        response = alice(voice('позвони егору'))
        _check_matched_contacts_analytics(response, [
            'b28f7f625a8c6fa6ff0c9',
            'eab9dad60848f4f41a0e6',
        ])
        _check_phone_book_analytics(response, [
            'eab9dad60848f4f41a0e6',
            'eab9dad60848f4f41a0e6',
            'eab9dad60848f4f41a0e6',
            'eab9dad60848f4f41a0e6',
            'b28f7f625a8c6fa6ff0c9',
            'b28f7f625a8c6fa6ff0c9',
            'b28f7f625a8c6fa6ff0c9',
            'b28f7f625a8c6fa6ff0c9',
        ],
        [
        ])

        response = alice(voice('пархоменко'))
        _check_matched_contacts_analytics(response, [
            'b28f7f625a8c6fa6ff0c9',
        ])

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_nlu_hints_single_phone(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)

        response = alice(voice('соседу'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_phone_call_nlg('Егор Сосед По Участку')
        assert layout.Directives[0].OpenUriDirective.Uri.startswith('tel:')

        _check_open_address_book_action_present(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_nlu_hints_multiple_phones(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)

        response = alice(voice('пархоменко'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == _make_choose_phone_nlg('Егор Мурманск Пархоменко')

        _check_open_address_book_action_present(response)

        assert len(response.run_response.ResponseBody.FrameActions) == 6
        for name, frame_action in response.run_response.ResponseBody.FrameActions.items():
            if name == OPEN_ADDRESS_BOOK:
                continue

            if name == '3':
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == 'Первый'
                continue

            if name == '4':
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == 'Второй'
                continue

            if name == '5':
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == 'Открой контакты'
                continue

            nlu_hint = frame_action.NluHint
            hint_number = int(nlu_hint.FrameName.split('_')[-1])
            hint_phrase = str(hint_number + 1)

            assert len(nlu_hint.Instances) == 2
            assert nlu_hint.Instances[0].Phrase == hint_phrase
            assert nlu_hint.Instances[1].Phrase == ORDINALS[hint_number]

        # Check voice command only once because nlu hints disappear after that
        response = alice(voice('1'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_phone_call_nlg('Егор Мурманск Пархоменко')
        assert layout.Directives[0].OpenUriDirective.Uri.startswith('tel:')

        _check_open_address_book_action_present(response)

    @pytest.mark.contacts(predefined_contacts.TWO_IVANS)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_nlu_hints_fullname(self, alice):
        response = alice(voice('позвони ивану'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)

        response = alice(voice('иван'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_phone_call_nlg('Иван')
        assert layout.Directives[0].OpenUriDirective.Uri.startswith('tel:')

        _check_open_address_book_action_present(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_contact_not_found(self, alice):
        response = alice(voice('позвони владимиру мономаху'))
        assert response.scenario_stages() == {'run'}

        body = response.run_response.ResponseBody

        assert body.Layout.OutputSpeech in CONTACT_NOT_FOUND_NLG

        _check_open_address_book_action_present(response)

        assert len(body.FrameActions) == 4
        assert body.FrameActions['1'].Directives.List[0].TypeTextDirective.Text == 'Позвони брату'
        assert body.FrameActions['2'].Directives.List[0].TypeTextDirective.Text == 'Открой контакты'
        assert body.FrameActions['3'].NluHint.FrameName == 'alice.phone_call.contact_from_address_book'

    @pytest.mark.parametrize('surface', [surface.maps])
    def test_list_phones_as_suggests(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)

        response = alice(voice('пархоменко'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == _make_choose_phone_nlg('Егор Мурманск Пархоменко')

        suggests = layout.SuggestButtons
        assert suggests[0].ActionButton.Title == '+7(469)483-19-64'
        assert suggests[0].ActionButton.ActionId == 'phone_call_0'
        assert suggests[1].ActionButton.Title == '+7(643)970 47 39'
        assert suggests[1].ActionButton.ActionId == 'phone_call_1'

        assert len(response.run_response.ResponseBody.FrameActions) == 4
        open_address_book_found = False

        for name, frame_action in response.run_response.ResponseBody.FrameActions.items():
            if name == OPEN_ADDRESS_BOOK:
                open_address_book_found = True
                continue

            if name == 'phone_call_0':
                assert frame_action.Callback.Name == 'choose_contact_callback'
                assert frame_action.Callback.Payload.fields['phone_id'].number_value == 41
                continue

            if name == 'phone_call_1':
                assert frame_action.Callback.Name == 'choose_contact_callback'
                assert frame_action.Callback.Payload.fields['phone_id'].number_value == 44
                continue

        assert open_address_book_found

    @pytest.mark.contacts(predefined_contacts.MANY_PETYAS)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_repeated_list_contacts(self, alice):
        response = alice(voice('позвони пете'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)
        _check_contacts_card(
            response,
            [
                ('Петя', '7a596bf26208d9ba6c710'),
                ('Петя Черный', 'ea29afc4a251ee9f9780b'),
                ('Петя Белый', '915738fad81221f619875'),
                ('Петя 2 Синий', '67bbf87e03652e157932f'),
                ('Петя 2 Черный', '5b9a8fa6d75fd85f429b1'),
            ],
        )

        response = alice(voice('черный'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)
        _check_contacts_card(
            response,
            [
                ('Петя Черный', 'ea29afc4a251ee9f9780b'),
                ('Петя 2 Черный', '5b9a8fa6d75fd85f429b1'),
            ],
        )

    @pytest.mark.contacts(predefined_contacts.MANY_PETYAS)
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_single_match_after_list_contacts(self, alice):
        response = alice(voice('позвони пете'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_open_address_book_action_present(response)
        _check_contacts_card(
            response,
            [
                ('Петя', '7a596bf26208d9ba6c710'),
                ('Петя Черный', 'ea29afc4a251ee9f9780b'),
                ('Петя Белый', '915738fad81221f619875'),
                ('Петя 2 Синий', '67bbf87e03652e157932f'),
                ('Петя 2 Черный', '5b9a8fa6d75fd85f429b1')
            ],
        )

        response = alice(voice('петя'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_phone_call_nlg('Петя')
        assert layout.Directives[0].OpenUriDirective.Uri.startswith('tel:')


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.contacts(predefined_contacts.TWO_EGORS)
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
@pytest.mark.parametrize('surface', [surface.station])
class TestPhoneCallUnsupportedAddressBook(TestPhoneCallBase):
    def test_unsupported_address_book(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}
        assert response.run_response.ResponseBody.AnalyticsInfo.Intent != 'phone_call'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.contacts(predefined_contacts.ONE_DIMA)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPhoneCallFuzzyMatch(TestPhoneCallBase):
    def test_no_call_on_fuzzy_match(self, alice):
        response = alice(voice('позвони дмитрию плотникову'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_contacts_div_card(
            response,
            [
                ('Дмитрий Королёв', 'ea29afc4a251ee9f9780b'),
            ],
        )
        assert len(response.run_response.ResponseBody.FrameActions) == 6


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.contacts(predefined_contacts.MANY_ARTYOMS)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
class TestPhoneCallListContacts(TestPhoneCallBase):
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_order_and_length(self, alice):
        response = alice(voice('позвони артёму'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        _check_contacts_div_card(
            response,
            [
                ('Артём 111111111', 'ea29afc4a251ee9f9780b'),
                ('Артём 11111', 'b77b30660d0c83a391e82'),
                ('Артём 11111111', '915738fad81221f619875'),
                ('Артём 111111', '67bbf87e03652e157932f'),
                ('Артём 11', '5a27568683d5b044911fc'),
            ],
        )

        _check_open_address_book_action_present(response)

    @pytest.mark.parametrize('surface', [surface.maps])
    def test_order_and_length_suggests(self, alice):
        response = alice(voice('позвони артёму'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        suggests = ['Артём 111', 'Артём 111111111', 'Артём 11111', 'Артём 11111111', 'Артём 111111']
        assert len(response.run_response.ResponseBody.FrameActions) == 18
        open_address_book_found = False
        for name, frame_action in response.run_response.ResponseBody.FrameActions.items():
            if name == OPEN_ADDRESS_BOOK:
                open_address_book_found = True
                continue

            if name in ['7', '8', '9', '10', '11']:
                assert len(frame_action.Directives.List) == 1
                assert frame_action.Directives.List[0].TypeTextDirective.Text == suggests[int(name)-7]
                continue
        assert open_address_book_found


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.contacts(predefined_contacts.MANY_ARTYOMS)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
@pytest.mark.experiments(
    'hw_phone_calls_stub_response',
)
class TestPhoneCallStubResponse(TestPhoneCallBase):
    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_order_and_length(self, alice):
        response = alice(voice('позвони артёму'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Нашла контакт в контактной книге. Звоню.'

        cards = response.run_response.ResponseBody.Layout.Cards
        assert len(cards) == 1
        assert cards[0].Text == 'Нашла контакт в контактной книге. Звоню.'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPhoneCallNoPermissions(TestPhoneCallBase):

    @pytest.mark.contacts(predefined_contacts.EMPTY_CONTACTS)
    def test_no_permissions(self, alice):
        response = alice(voice("позвони егору"))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.Cards[0].Text == 'С радостью позвоню, только предоставьте мне доступ к контактной книге.'
        assert layout.OutputSpeech == ''
        assert len(layout.Directives) == 1
        request_permissions_directive = layout.Directives[0].RequestPermissionsDirective
        assert request_permissions_directive.Permissions == [
            TPermissions.ReadContacts,
        ]

        _check_open_address_book_action_present(response)

        on_success_callback = _parse_callback(request_permissions_directive.OnSuccess.CallbackDirective)
        on_fail_callback = _parse_callback(request_permissions_directive.OnFail.CallbackDirective)

        response = alice(on_success_callback)
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Отлично! Дайте мне пару минут на изучение ваших контактов.'

        _check_open_address_book_action_present(response)

        response = alice(on_fail_callback)
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'К сожалению, без вашего разрешения тут я вам помочь не смогу.'

        _check_open_address_book_action_present(response)

    def test_no_permissions_no_contacts(self, alice):
        response = alice(voice("позвони егору"))

        layout = response.run_response.ResponseBody.Layout
        assert layout.Cards[0].Text == 'С радостью позвоню, только предоставьте мне доступ к контактной книге.'
        request_permissions_directive = layout.Directives[0].RequestPermissionsDirective
        on_success_callback = _parse_callback(request_permissions_directive.OnSuccess.CallbackDirective)
        on_fail_callback = _parse_callback(request_permissions_directive.OnFail.CallbackDirective)

        response = alice(on_success_callback)
        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Отлично! Дайте мне пару минут на изучение ваших контактов.'

        response = alice(on_fail_callback)
        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'К сожалению, без вашего разрешения тут я вам помочь не смогу.'

    @pytest.mark.contacts(predefined_contacts.EMPTY_CONTACTS)
    def test_waiting_for_contacts(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.Cards[0].Text == 'С радостью позвоню, только предоставьте мне доступ к контактной книге.'
        assert layout.OutputSpeech == ''
        assert len(layout.Directives) == 1
        request_permissions_directive = layout.Directives[0].RequestPermissionsDirective
        assert request_permissions_directive.Permissions == [
            TPermissions.ReadContacts,
        ]

        _check_open_address_book_action_present(response)

        alice.grant_permissions('read_contacts')

        replies = [
            "Пока дошла только до буквы 'К', нужно еще немного времени.",
            "Уже на букве 'Р', скоро буду готова.",
            "Обрабатываю контакты на букву 'Ф'. Пожалуйста, подождите.",
            "Не смогла обработать вашу контактную книгу, через некоторое время попробую еще раз.",
        ] + [
            "Пока что я умею звонить только в экстренные службы. " +
            "Извините, но синхронизироваться с Вашей контактной книгой мне так и не удалось.",
        ] * 2

        _test_repeated_query(alice, replies)

    @pytest.mark.contacts(predefined_contacts.EMPTY_CONTACTS)
    @pytest.mark.experiments('enable_outgoing_device_calls')
    def test_waiting_for_contacts_with_enabled_device_calls(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.Cards[0].Text == 'С радостью позвоню, только предоставьте мне доступ к контактной книге.'
        assert layout.OutputSpeech == ''
        assert len(layout.Directives) == 1
        request_permissions_directive = layout.Directives[0].RequestPermissionsDirective
        assert request_permissions_directive.Permissions == [
            TPermissions.ReadContacts,
        ]

        _check_open_address_book_action_present(response)

        alice.grant_permissions('read_contacts')

        replies = [
            "Пока дошла только до буквы 'К', нужно еще немного времени.",
            "Уже на букве 'Р', скоро буду готова.",
            "Обрабатываю контакты на букву 'Ф'. Пожалуйста, подождите.",
            "Не смогла обработать вашу контактную книгу, через некоторое время попробую еще раз.",
        ] + [
            "Пока что я умею звонить только в экстренные службы и на умные Станции Яндекса. " +
            "Извините, но синхронизироваться с Вашей контактной книгой мне так и не удалось.",
        ] * 2

        _test_repeated_query(alice, replies)


@pytest.mark.supported_features('phone_address_book')
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPhoneCallUnauthorized(TestPhoneCallBase):
    def test_unauthorized(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Для звонков по телефону вам нужно авторизоваться в приложении.'
        assert len(layout.Directives) == 1
        assert layout.Directives[0].OpenUriDirective.Uri == 'yandex-auth://?theme=light'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='MessengerCall', handle='messenger_call')
@pytest.mark.contacts(predefined_contacts.TWO_EGORS)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
@pytest.mark.experiments(
    'hw_enable_phone_calls',
    'mm_enable_begemot_contacts',
    'vins_add_irrelevant_intents=personal_assistant.scenarios.call',
)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestCallToFrame:
    def test_messenger_call(self, alice):
        response = alice(voice('позвони'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in CONTACT_NOT_FOUND_NLG

        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'

        response = alice(voice('позвони в скорую'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert layout.Directives[0].OpenUriDirective.Uri.startswith('tel:103')


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.contacts(predefined_contacts.TWO_NAMESAKE_EGORS)
@pytest.mark.supported_features('phone_address_book')
@pytest.mark.additional_options(permissions=[
    {
        'name': 'read_contacts',
        'granted': True
    },
])
class TestNamesakeContacts(TestPhoneCallBase):

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_namesake_contacts(self, alice):
        response = alice(voice('позвони егору'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Выберите, кому звонить.'
        assert len(layout.SuggestButtons) == 3

        frame_actions = response.run_response.ResponseBody.FrameActions
        assert len(frame_actions) == 9

        _check_contacts_card(
            response,
            [
                ('Егор Мурманск Пархоменко', 'eab9dad60848f4f41a0e6'),
                ('Егор Мурманск Пархоменко', 'b28f7f625a8c6fa6ff0c9'),
            ],
        )


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('hw_disable_device_call_shortcut', 'enable_outgoing_device_to_device_calls')
class TestDeviceToDeviceCall(TestDeviceCallBase):

    @pytest.mark.device_state(device_id=STATION3_ID)
    def test_device_to_device_call_multiple(self, alice):
        response = alice(voice('позвони в спальню'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech.startswith('Не понимаю, на какое устройство нужно позвонить. Уточните название, пожалуйста.')

        _check_push_directive(response.run_response.ResponseBody.ServerDirectives)

    @pytest.mark.device_state(device_id=STATION3_ID)
    def test_device_to_device_call_single(self, alice):
        response = alice(voice('позвони на станцию мини под названием грабли'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_messenger_call_directive(layout.Directives, STATION6_ID)

        assert layout.OutputSpeech in _make_device_call_nlg("Грабли - Коридор")

    @pytest.mark.device_state(device_id=STATION1_ID)
    def test_device_to_device_call_exclude_caller(self, alice):
        response = alice(voice('позвони в дом в спальню'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_messenger_call_directive(layout.Directives, STATION2_ID)

        assert layout.OutputSpeech in _make_device_call_nlg("Стас - Спальня")

    @pytest.mark.device_state(device_id=STATION1_ID)
    def test_device_to_device_call_callable(self, alice):
        response = alice(voice('позвони на кухню'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_messenger_call_directive(layout.Directives, STATION3_ID)

        assert layout.OutputSpeech in _make_device_call_nlg("Гена - Кухня")

    @pytest.mark.device_state(device_id=STATION1_ID)
    def test_device_to_device_no_such_callable(self, alice):
        response = alice(voice('позвони в прихожую'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_push_directive(response.run_response.ResponseBody.ServerDirectives)

        assert layout.OutputSpeech.startswith('Не нашла устройство, на которое можно было бы позвонить. Отправила ссылку')

    @pytest.mark.device_state(device_id=STATION1_ID)
    def test_device_to_device_no_such_device(self, alice):
        response = alice(voice('позвони на станцию макс'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_push_directive(response.run_response.ResponseBody.ServerDirectives)

        assert layout.OutputSpeech.startswith('Не понимаю, на какое устройство нужно позвонить. Уточните название, пожалуйста. Отправила ссылку')

    @pytest.mark.device_state(messenger_call={
        "incoming": {
            "call_id": "call_0",
            "caller_name": "Petya",
        }
    })
    def test_device_to_device_get_caller_name_station(self, alice):
        payload = GET_CALLER_NAME_SEMANTIC_FRAME
        payload["typed_semantic_frame"]["get_caller_name"]["caller_device_id"]["string_value"] = STATION2_ID

        response = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Звонок с Яндекс станции.'

    @pytest.mark.device_state(messenger_call={
        "incoming": {
            "call_id": "call_0",
            "caller_name": "Petya",
        }
    })
    def test_device_to_device_get_caller_name_person(self, alice):
        payload = GET_CALLER_NAME_SEMANTIC_FRAME
        payload["typed_semantic_frame"]["get_caller_name"]["caller_device_id"]["string_value"] = "dishwasher"

        response = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Вам звонит Petya.'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.experiments('hw_disable_device_call_shortcut')
class TestAppToDeviceCall(TestDeviceCallBase):

    def test_device_call_multiple(self, alice):
        response = alice(voice('позвони в спальню'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Не понимаю, на какое устройство нужно позвонить. Уточните название, пожалуйста.'
        assert len(layout.Cards) == 1

        _check_open_quasar_screen_directive(response.run_response.ResponseBody.Layout.Directives)

    def test_device_call_single(self, alice):
        response = alice(voice('позвони на станцию мини дома'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_device_call_nlg('Стас - Спальня')

        _check_openuri_directive(layout.Directives, 'Стас - Спальня', STATION2_ID)

    def test_device_call_no_such_callable(self, alice):
        response = alice(voice('позвони в прихожую'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_open_quasar_screen_directive(response.run_response.ResponseBody.Layout.Directives)

        assert layout.OutputSpeech == 'Не нашла устройство, на которое можно было бы позвонить. Проверьте список Ваших устройств.'

    def test_device_call_no_such_device(self, alice):
        response = alice(voice('позвони на яндекс станцию макс'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_open_quasar_screen_directive(response.run_response.ResponseBody.Layout.Directives)

        assert layout.OutputSpeech == 'Не понимаю, на какое устройство нужно позвонить. Уточните название, пожалуйста.'

    def test_device_call_household(self, alice):
        response = alice(voice('позвони на дачу в спальню'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech in _make_device_call_nlg('Цветок - Спальня')

        _check_openuri_directive(layout.Directives, 'Цветок - Спальня', STATION8_ID)


@pytest.mark.parametrize('surface', [surface.webtouch])
class TestsWebtouch(TestDeviceCallBase):
    @pytest.mark.contacts(predefined_contacts.EMPTY_CONTACTS)
    @pytest.mark.experiments('enable_outgoing_device_calls')
    def test_show_promo(self, alice):
        response = alice(voice('позвони егору'))
        import sys
        print(response.run_response.ResponseBody, file=sys.stderr)
        assert response.scenario_stages() == {'run'}
        assert response.run_response.ResponseBody.Layout.OutputSpeech
        assert response.run_response.ResponseBody.Layout.Directives[0].ShowPromoDirective


def create_nanny_call_payload():
    call_payload = TCallPayload()
    call_payload.CallToNanny = True
    return base64.b64encode(call_payload.SerializeToString()).decode('ascii')


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('hw_disable_device_call_shortcut', 'enable_outgoing_device_to_device_calls', 'hw_enable_call_to_nanny_entry')
class TestCallToNanny(TestDeviceCallBase):

    @pytest.mark.device_state(device_id=STATION3_ID)
    def test_device_to_device_call_single(self, alice):
        response = alice(voice('позвони на станцию мини под названием грабли в режиме радио няни'))
        assert response.scenario_stages() == {'run'}

        layout = response.run_response.ResponseBody.Layout

        _check_messenger_call_directive(layout.Directives, STATION6_ID)
        call_directive = response.run_response.ResponseBody.Layout.Directives[0].MessengerCallDirective
        assert call_directive.CallToRecipient.Payload == create_nanny_call_payload()
        return str(response)

    @pytest.mark.device_state(messenger_call={
        "incoming": {
            "call_id": "call_0",
            "caller_name": "Petya",
        }
    })
    def test_device_to_device_get_caller_name_station(self, alice):
        payload = GET_CALLER_NAME_SEMANTIC_FRAME
        payload["typed_semantic_frame"]["get_caller_name"]["caller_device_id"]["string_value"] = STATION2_ID

        call_payload = TCallPayload()
        call_payload.CallToNanny = True
        payload["typed_semantic_frame"]["get_caller_name"]["caller_payload"] = {}
        payload["typed_semantic_frame"]["get_caller_name"]["caller_payload"]["string_value"] = create_nanny_call_payload()

        response = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert response.scenario_stages() == {'run'}

        directives = response.run_response.ResponseBody.Layout.Directives
        assert directives[-1].MessengerCallDirective is not None
        assert directives[-1].MessengerCallDirective.AcceptCall is not None

        assert directives[0].SoundMuteDirective is not None
        return str(response)
