import re

import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import call.contacts as contacts


supported_phone_call_surfaces = [surface.searchapp, surface.maps]
not_supported_phone_call_surfaces = [x for x in surface.actual_surfaces if x not in supported_phone_call_surfaces]


def _make_phone_call_nlg(name):
    return {
        f'{name}, набираю.',
        f'{name}, звоню.',
        f'{name}, уже набираю.',
        f'{name}, сейчас позвоним.',
    }


def _check_contact_div_card_header(div_card):
    assert div_card.all_contacts_text == 'Все контакты'
    assert div_card.all_contacts_action.log_id
    assert re.match(
        r'dialog-action://\?directives=.*?open_uri.*?contacts%3A//address_book',
        div_card.all_contacts_action.url,
    )


def _check_contact_div_card_contents(actual_contacts, expected_contacts):
    assert len(actual_contacts) == len(expected_contacts)
    for actual_contact, expected_contact in zip(actual_contacts, expected_contacts):
        assert actual_contact.profile_name == expected_contact[0]
        assert actual_contact.action.log_id
        assert re.match(
            r'dialog-action://\?directives=.*?choose_contact_callback.*?' + expected_contact[1],
            actual_contact.action.url,
        )


def _check_phones_div_card_contents(actual_phones, expected_phones):
    assert len(actual_phones) == len(expected_phones)
    for actual_phone, expected_phone in zip(actual_phones, expected_phones):
        assert actual_phone.phone_number == expected_phone[0]
        assert actual_phone.action.log_id
        assert re.match(
            r'dialog-action://\?directives=.*?choose_contact_callback.*?' + expected_phone[1],
            actual_phone.action.url,
        )


def _check_suggests_contents(suggests, expected_contents):
    assert len(suggests) == len(expected_contents) + 1
    open_address_book_suggest = False
    actual_contents = []
    for suggest in suggests:
        if suggest.title == 'Открой контакты':
            open_address_book_suggest = True
        else:
            actual_contents.append(suggest.title)
    assert open_address_book_suggest
    assert set(actual_contents) == set(expected_contents)


class _TestMessengerCallBase(object):
    owners = ('deemonasd', 'flimsywhimsy', 'sdll')


@pytest.mark.parametrize('surface', not_supported_phone_call_surfaces)
class TestMessengerCall(_TestMessengerCallBase):

    def _is_emergency_callable(self, alice):
        return (surface.is_launcher(alice) or
                surface.is_navi(alice))

    def test_ambulance(self, alice):
        response = alice('позвони в скорую')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.EmergencyCall
        if self._is_emergency_callable(alice):
            assert len(response.directives) == 1
            assert response.directive.name == directives.names.OpenUriDirective
            assert response.directive.payload.uri in ['tel:103', 'yandexnavi://dial_phone?phone_number=103']
        else:
            assert not response.directives

    def test_mom(self, alice):
        response = alice('позвони маме')
        assert response.scenario == scenario.MessengerCall
        assert response.intent is None
        if self._is_emergency_callable(alice):
            assert response.text == 'Пока что я умею звонить только в экстренные службы.'
        elif surface.is_watch(alice):
            assert response.text in [
                'Я научусь звонить с часов, но пока не умею.',
                'С этим в часах пока не помогу. Но только пока.',
                'Я бы и рада, но ещё не научилась. Всё будет.',
            ]
        else:
            assert response.text in [
                'Я справлюсь с этим лучше на телефоне.',
                'Это я могу, но лучше с телефона.',
                'Для звонков телефон как-то удобнее, давайте попробую там.',
            ]


@pytest.mark.usefixtures('reset_session')
@pytest.mark.app(uuid='ffffffffffffffffb4b76ceec228a278')
@pytest.mark.permissions('read_contacts')
class _TestPhoneCallBase(_TestMessengerCallBase):
    pass


@pytest.mark.oauth(auth.RobotPhoneCaller)
@pytest.mark.parametrize('surface', supported_phone_call_surfaces)
class TestMessengerCallWithPhoneCalls(_TestPhoneCallBase):

    def test_ambulance(self, alice):
        response = alice('позвони в скорую')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.EmergencyCall
        assert len(response.directives) == 1
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri.startswith('tel:103')

    @pytest.mark.supported_features('phone_address_book')
    def test_phone_call(self, alice):
        response = alice('позвони егору')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall

        assert len(response.cards) == 1
        if surface.is_searchapp(alice):
            assert response.div_card
            _check_contact_div_card_header(response.div_card.contacts_card)
            _check_contact_div_card_contents(
                response.div_card.contacts_card.contacts,
                [
                    ('Егор Мурманск Пархоменко', 'b28f7f625a8c6fa6ff0c9'),
                    ('Егор Сосед По Участку', 'eab9dad60848f4f41a0e6'),
                ],
            )
        else:
            assert response.text == "Выберите, кому звонить."
            _check_suggests_contents(
                response.suggests,
                [
                    'Егор Мурманск Пархоменко',
                    'Егор Сосед По Участку',
                ],
            )

        response = alice('пархомченко')

        assert response.scenario == scenario.MessengerCall
        assert len(response.cards) == 1

        if surface.is_searchapp(alice):
            assert response.div_card
            assert response.div_card.contact_card.profile_name == 'Егор Мурманск Пархоменко'
            _check_phones_div_card_contents(
                response.div_card.contact_card.contacts,
                [
                    ('+74694831964', '40'),
                    ('+76439704739', '44'),
                ],
            )
        else:
            assert response.text == 'Егор Мурманск Пархоменко. Выберите, на какой номер звонить.'
            _check_suggests_contents(
                response.suggests,
                [
                    '+74694831964',
                    '+76439704739',
                ],
            )

        response = alice('второй')
        assert response.scenario == scenario.MessengerCall
        assert response.text in _make_phone_call_nlg('Егор Мурманск Пархоменко')
        assert response.directive.payload.uri == 'tel:+76439704739'


@pytest.mark.oauth(auth.RobotPhoneCaller)
@pytest.mark.supported_features('outgoing_phone_calls')
@pytest.mark.parametrize('surface', [surface.navi])
class TestAddressBookSupportedFeatureAbsent(_TestPhoneCallBase):
    def test_phone_call(self, alice):
        response = alice('позвони егору')
        assert response.scenario == scenario.MessengerCall
        assert not response.intent
        assert response.text == 'Пока что я умею звонить только в экстренные службы.'


@pytest.mark.parametrize('surface', supported_phone_call_surfaces)
class TestPhoneCallFat(_TestPhoneCallBase):

    @pytest.mark.oauth(auth.RobotPhoneCallerFat4k)
    def test_phone_call_4k(self, alice):
        response = alice('позвони артему 1234')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall

        assert response.text in _make_phone_call_nlg('Артём 1234')

    @pytest.mark.oauth(auth.RobotPhoneCallerFat8k)
    def test_phone_call_8k(self, alice):
        response = alice('позвони диме 5713')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall

        assert len(response.cards) == 1
        if surface.is_searchapp(alice):
            if response.div_card:
                has_dima = False
                for contact in response.div_card.contacts_card.contacts:
                    has_dima |= contact.profile_name.startswith('Дима')
                assert has_dima
            else:
                assert response.text in _make_phone_call_nlg('Дима 5713')
        else:
            if response.text == 'Выберите, кому звонить.':
                has_dima = False
                for suggest in response.suggests:
                    has_dima |= suggest.title.startswith('Дима')
                assert has_dima
            else:
                assert response.text in _make_phone_call_nlg('Дима 5713')


@pytest.mark.experiments('use_contacts')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', supported_phone_call_surfaces)
class TestPredefinedContacts(_TestPhoneCallBase):

    @pytest.mark.predefined_contacts(contacts.TwoNamesakeEgors)
    def test_phone_call(self, alice):
        response = alice('позвони егору')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall

        assert len(response.cards) == 1
        if surface.is_searchapp(alice):
            assert response.div_card
            _check_contact_div_card_header(response.div_card.contacts_card)
            _check_contact_div_card_contents(
                response.div_card.contacts_card.contacts,
                [
                    ('Егор Мурманск', 'eab9dad60848f4f41a0e6'),
                    ('Егор Мурманск Пархоменко', 'b28f7f625a8c6fa6ff0c9'),
                ],
            )
        else:
            assert response.text == 'Выберите, кому звонить.'
            _check_suggests_contents(
                response.suggests,
                [
                    'Егор Мурманск',
                    'Егор Мурманск Пархоменко'
                ],
            )

        response = alice('первый')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall

        assert response.text in _make_phone_call_nlg('Егор Мурманск')
        assert response.directive.payload.uri == 'tel:+76223099129'
        assert not response.div_card

    @pytest.mark.predefined_contacts(contacts.ThreeSashas)
    def test_ranking(self, alice):
        response = alice('позвони саше')
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall

        if surface.is_searchapp(alice):
            assert response.div_card
            _check_contact_div_card_header(response.div_card.contacts_card)
            _check_contact_div_card_contents(
                response.div_card.contacts_card.contacts,
                [
                    ('Саша', 'aaa'),
                    ('Саша Белый', 'ccc'),
                    ('Саша 2 Белый', 'bbb'),
                ],
            )
        else:
            assert response.text == 'Выберите, кому звонить.'
            _check_suggests_contents(
                response.suggests,
                [
                    'Саша',
                    'Саша Белый',
                    'Саша 2 Белый',
                ],
            )


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestDeviceCallShortcut(object):

    owners = ('akastornov', )

    @pytest.mark.parametrize('command', [
        pytest.param('позвони в колонку бесплатно', id='overriden'),
        pytest.param('позвони в колонку', id='not_overriden'),
    ])
    def test_shortcut(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.DeviceShortcut
        if surface.is_searchapp(alice):
            assert response.text == 'Без проблем! Давайте выберем, в какую?'
            assert response.directive.name == directives.names.OpenUriDirective
            assert response.directive.payload.uri == 'opensettings://?screen=quasar'
        else:
            assert response.text == 'К сожалению, не могу открыть страницу устройств Яндекса здесь.'
            assert not response.directives
