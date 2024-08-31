import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


safe_surfaces = [surface.station(device_config={'content_settings': content_settings_pb2.safe})]


def _assert_has_directive(response, directive_name):
    assert response.directives
    assert any(
        directive.name == directive_name
        for directive in response.directives
    )


def _check_time(response):
    assert response.intent == intent.GetTime
    assert response.directives
    assert response.directives[-1].name == directives.names.MmStackEngineGetNextCallback


def _check_scenario(response, scenario, prefix=None):
    assert response.scenario == scenario
    assert response.directives
    assert response.directives[-1].name == directives.names.MmStackEngineGetNextCallback
    if prefix:
        assert response.text.startswith(prefix)


def _check_alice_show(response, check_gif):
    _check_scenario(response, scenario.AliceShow)
    if check_gif and response.text:
        _assert_has_directive(response, directives.names.DrawLedScreenDirective)


def _day_part(dt):
    if 5 <= dt.hour <= 17:
        return 'morning'
    if 17 < dt.hour < 21:
        return 'evening'
    return 'night'


def _age(device_state):
    if device_state.DeviceConfig.ContentSettings == content_settings_pb2.safe:
        return 'children'
    return 'adult'


# show_type will be obsolete someday
def _show_type(day_part, age):
    if age == 'children':
        if day_part == 'morning':
            return 'children'
        return 'children_night'
    return day_part


@pytest.mark.experiments(
    f'mm_enable_protocol_scenario={scenario.AliceShow}',
)
class TestAliceShowBase(object):
    owners = ('olegator', 'lavv17', 'flimsywhimsy')


@pytest.mark.version(hollywood=211)
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_alice_show_new_user')
@pytest.mark.surface(surface.actual_surfaces, exclude=surface.smart_tv)
class TestAliceShowMorning(TestAliceShowBase):

    @pytest.mark.parametrize('command, dummy_response', [
        ('запусти утреннее шоу', True),
        ('доброе утро', False),
    ])
    def test_morning_show(self, alice, command, dummy_response):
        response = alice(command)

        if 'directive_sequencer' not in alice.supported_features:
            assert not response.directive
            if dummy_response:
                assert response.scenario == scenario.AliceShow
                assert 'не могу' in response.text
            else:
                assert response.intent == intent.GoodMorning
            return

        check_gif = 'led_display' in alice.supported_features

        _check_alice_show(response, check_gif)
        assert response.scenario_analytics_info.object('name') == 'morning'

        response.next()
        _check_alice_show(response, check_gif)

        response.next()
        _check_time(response)

        response.next()
        _check_alice_show(response, check_gif)

        response.next()
        _check_scenario(response, scenario.Weather, 'Сейчас в ')

        response.next()
        _check_alice_show(response, False)

        response = alice('предыдущий трек')
        _check_alice_show(response, check_gif)

        response.next()
        _check_scenario(response, scenario.Weather, 'Сейчас в ')

        # hardcoded show begins
        response.next()
        _check_alice_show(response, check_gif)
        response.next()
        assert response.scenario == scenario.HollywoodHardcodedMusic
        assert response.intent == intent.MorningShow
        _assert_has_directive(response, directives.names.MusicPlayDirective)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_enable_evening_show_good_evening')
@pytest.mark.surface(surface.actual_surfaces, exclude=surface.smart_tv)
class TestAliceShowEvening(TestAliceShowBase):

    @pytest.mark.parametrize('command, dummy_response', [
        ('запусти вечернее шоу', True),
        ('добрый вечер', False),
    ])
    def test_evening_show(self, alice, command, dummy_response):
        response = alice(command)

        if 'directive_sequencer' not in alice.supported_features:
            assert not response.directive
            if dummy_response:
                assert response.scenario == scenario.AliceShow
                assert 'не могу' in response.text
            else:
                assert response.intent == intent.Hello
            return

        check_gif = 'led_display' in alice.supported_features

        _check_alice_show(response, check_gif)
        assert response.scenario_analytics_info.object('name') == 'evening'

        response.next()
        _check_alice_show(response, check_gif)

        response.next()
        _check_time(response)

        response.next()
        _check_alice_show(response, check_gif)

        response.next()
        _check_scenario(response, scenario.Weather, 'Завтра в ')


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hw_enable_good_night_show_good_night')
@pytest.mark.surface(surface.actual_surfaces, exclude=surface.smart_tv)
class TestAliceShowGoodNight(TestAliceShowBase):

    @pytest.mark.parametrize('command, dummy_response', [
        ('запусти ночное шоу', True),
        ('спокойной ночи', False),
    ])
    def test_good_night(self, alice, command, dummy_response):
        response = alice(command)

        if 'directive_sequencer' not in alice.supported_features:
            assert not response.directive
            if dummy_response:
                assert response.scenario == scenario.AliceShow
                assert 'не могу' in response.text
            else:
                assert response.intent == intent.GoodNight
            return

        check_gif = 'led_display' in alice.supported_features

        _check_alice_show(response, check_gif)
        assert response.scenario_analytics_info.object('name') == 'night'
        assert not response.text

        response.next()
        assert response.scenario == scenario.HollywoodMusic
        _assert_has_directive(response, directives.names.AudioPlayDirective)
        assert not response.text


@pytest.mark.no_oauth
@pytest.mark.experiments(
    'hw_enable_evening_show_good_evening',
    'hw_enable_good_night_show_good_night',
    'hw_disable_alice_show_without_music',
)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestAliceShowNoPlus(TestAliceShowBase):

    def test_alice_show_direct(self, alice):
        response = alice('включай шоу')

        assert not response.directive
        assert response.scenario == scenario.AliceShow

        if surface.is_smart_speaker(alice):
            assert 'Плюс' in response.text
        else:
            assert 'не могу' in response.text

    def test_morning_show_proactive(self, alice):
        response = alice('доброе утро')
        assert response.intent == intent.GoodMorning

    def test_evening_show_proactive(self, alice):
        response = alice('добрый вечер')
        assert response.intent == intent.Hello

    def test_night_show_proactive(self, alice):
        response = alice('спокойной ночи')
        assert response.intent == intent.GoodNight


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'hw_enable_evening_show_good_evening',
    'hw_enable_good_night_show_good_night',
)
# TODO(lavv17): add safe_surfaces when children morning show works in safe mode
@pytest.mark.surface(surface.actual_surfaces, exclude=surface.smart_tv)
class TestAliceShowWithDayPartAndAge(TestAliceShowBase):

    @pytest.mark.parametrize('command, day_part, age', [
        ('запусти детское утреннее шоу', 'morning', 'children'),
        ('запусти детское вечернее шоу', 'evening', 'children'),
        ('запусти детское ночное шоу', 'night', 'children'),
        ('детское шоу', None, 'children'),
        ('запусти утреннее шоу', 'morning', None),
        ('запусти вечернее шоу', 'evening', None),
        ('запусти ночное шоу', 'night', None),
        ('запусти взрослое ночное шоу', 'night', 'adult'),
        ('взрослое шоу', None, 'adult'),
        ('включи шоу', None, None),
    ])
    def test_show_kinds(self, alice, command, day_part, age):
        response = alice(command)

        if day_part is None:
            day_part = _day_part(alice.datetime_now)
        if age is None:
            age = _age(alice.device_state)
        show_type = _show_type(day_part, age)
        check_gif = 'led_display' in alice.supported_features

        if 'directive_sequencer' not in alice.supported_features:
            assert not response.directive
            assert response.scenario == scenario.AliceShow
            assert 'не могу' in response.text
            return

        _check_alice_show(response, check_gif and show_type != 'children')
        assert response.scenario_analytics_info.objects['show.type']['name'] == show_type
        assert response.scenario_analytics_info.objects['show.day_part']['name'] == day_part
        assert response.scenario_analytics_info.objects['show.age']['name'] == age

        response.next()
        if show_type == 'children':
            assert response.scenario == scenario.HollywoodHardcodedMusic
            assert response.intent == intent.MorningShow
            _assert_has_directive(response, directives.names.MusicPlayDirective)
        elif show_type == 'night':
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_directive(response, directives.names.AudioPlayDirective)
            assert not response.text
        elif show_type == 'children_night':
            assert response.scenario == scenario.HollywoodMusic
            _assert_has_directive(response, directives.names.AudioPlayDirective)
            assert response.text in (
                'Включаю Подушки-шоу.',
                'Продолжаю Подушки-шоу с момента, на котором мы остановились. Чтобы включить заново, скажи: "Алиса, включи сначала".',
            )
        else:
            _check_alice_show(response, check_gif)

    @pytest.mark.parametrize('command', [
        ('вечернее утреннее шоу'),
        ('детское шоу для взрослых'),
    ])
    def test_ambiguous_show_type(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.AliceShow
        assert not response.directive
        if 'directive_sequencer' not in alice.supported_features:
            assert 'не могу' in response.text
        else:
            assert response.text == 'Простите, я не поняла, какое Шоу вы хотите послушать.'


@pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEPRODUCT-464, need user with smi news source selected in memento')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'hw_enable_alice_show_interactivity',
    'hw_alice_show_interactivity_cooldown=0',
    'hw_alice_show_interactivity_rejection_limit=1000000',
)
@pytest.mark.surface(surface.actual_surfaces, exclude=surface.smart_tv)
class TestAliceShowInteractivity(TestAliceShowBase):

    def test_interactivity_pass(self, alice):
        response = alice('утреннее шоу')

        if 'directive_sequencer' not in alice.supported_features:
            assert 'не могу' in response.text
            return

        check_gif = 'led_display' in alice.supported_features

        # Show sequence without interactivity
        _check_alice_show(response, check_gif)

        response.next()
        _check_alice_show(response, check_gif)
        assert 'Хотите сегодня послушаем' in response.text
        response.next()
        _check_alice_show(response, check_gif)
        assert 'как обычно' in response.text

        response.next()
        _check_alice_show(response, check_gif)
        response.next()
        _check_time(response)

        response.next()
        _check_alice_show(response, check_gif)

        response.next()
        _check_scenario(response, scenario.Weather, 'Сейчас в ')

    def test_interactivity_yes(self, alice):
        response = alice('утреннее шоу')

        if 'directive_sequencer' not in alice.supported_features:
            assert 'не могу' in response.text
            return

        check_gif = 'led_display' in alice.supported_features

        # Show sequence without interactivity
        _check_alice_show(response, check_gif)

        response.next()
        _check_alice_show(response, check_gif)
        assert 'Хотите сегодня послушаем' in response.text
        response = alice('хочу')
        _check_alice_show(response, check_gif)
        assert 'Хорошо' in response.text

        response.next()
        _check_alice_show(response, check_gif)
        response.next()
        _check_time(response)

        response.next()
        _check_alice_show(response, check_gif)

        response.next()
        _check_scenario(response, scenario.Weather, 'Сейчас в ')
