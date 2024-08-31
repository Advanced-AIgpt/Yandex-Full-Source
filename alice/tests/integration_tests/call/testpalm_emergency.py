import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def _assert_phone_directive_uri(response, uri):
    assert any(calling in response.text for calling in [
        'Набираю',
        'Звоню',
        'Уже набираю',
        'Сейчас позвоним',
    ])

    assert response.directive.name == directives.names.OpenUriDirective
    assert response.directive.payload.uri == uri


class TestPalmEmergency(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-9
    https://testpalm.yandex-team.ru/testcase/alice-1184
    https://testpalm.yandex-team.ru/testcase/alice-1365
    https://testpalm.yandex-team.ru/testcase/alice-1550
    https://testpalm.yandex-team.ru/testcase/alice-2172
    """

    owners = ('sparkle',)

    commands_and_phones = [
        ('позвони пожарным', '101'),
        ('вызови МЧС', '101'),
        ('вызови быстрее, пожалуйста, полицию', '102'),
        ('вызови полицию', '102'),
        ('позвони в милицию', '102'),
        ('позвони в скорую', '103'),
    ]

    @pytest.mark.parametrize('surface', [
        surface.automotive,
        surface.yabro_win,
    ])
    @pytest.mark.parametrize('command, phone', commands_and_phones)
    def test_call_emergency_stub(self, alice, command, phone):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.EmergencyCall

        assert phone in response.text
        assert any(call_asap in response.text for call_asap in [
            'звоните скорее со своего телефона',
            'наберите на телефоне',
            'наберите скорее со своего телефона',
            'звоните скорее с телефона',
            'скорее, я за вас переживаю',
        ])

    @pytest.mark.parametrize('surface', [surface.watch])
    @pytest.mark.parametrize('command, phone', commands_and_phones)
    def test_call_emergency_elari_stub(self, alice, command, phone):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.EmergencyCall

        assert phone in response.text
        assert any(do_asap in response.text for do_asap in [
            'Если что-то случилось, нажми и держи яркую кнопку. Родители получат сигнал.',
            'Звонить я пока не умею. Если что-то не так, нужно долго держать яркую кнопку.',
            'Пока не умею. Но если срочно нужна помощь, нажми и держи яркую кнопку.',
        ])

    # call 101, 102 or 103 in Russia
    @pytest.mark.parametrize('surface, uri_template', [
        (surface.searchapp, 'tel:{}'),
        (surface.navi, 'yandexnavi://dial_phone?phone_number={}'),
    ])
    @pytest.mark.parametrize('command, phone', commands_and_phones)
    def test_call_emergency_russian(self, alice, command, phone, uri_template):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.EmergencyCall
        _assert_phone_directive_uri(response, uri_template.format(phone))

    # call only 112 outside Russia
    @pytest.mark.region(region.Berlin)
    @pytest.mark.parametrize('surface, uri_template', [
        (surface.searchapp, 'tel:112'),
        (surface.navi, 'yandexnavi://dial_phone?phone_number=112'),
    ])
    @pytest.mark.parametrize('command, phone', commands_and_phones)
    def test_call_emergency_foreign(self, alice, command, phone, uri_template):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.EmergencyCall
        _assert_phone_directive_uri(response, uri_template)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command', [
        'позвони 8(916)123-45-67',
        'позвони Антону'
    ])
    def test_call_emergency_fail(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MessengerCall
        assert response.intent == intent.PhoneCall
        assert response.text == 'Для звонков по телефону вам нужно авторизоваться в приложении.'
