import re
from urllib.parse import unquote

import pytest

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.surface as surface
from .common import PogodaDialogBase, WeatherDivCard, get_day, get_weekday, get_month

NOW_PHRASES = r'([сС]ейчас|[вВ] настоящее время|[вВ] настоящий момент|[вВ] данный момент|[вВ] данную минуту)'


class TestPalmPogodaCommon(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-582
    https://testpalm.yandex-team.ru/testcase/alice-626
    https://testpalm.yandex-team.ru/testcase/alice-636

    https://testpalm.yandex-team.ru/testcase/alice-1083

    https://testpalm.yandex-team.ru/testcase/alice-415
    https://testpalm.yandex-team.ru/testcase/alice-629

    Частично https://testpalm.yandex-team.ru/testcase/alice-1507
    """

    commands = [
        # 1st block (alice-582, alice-626, alice-636)
        (
            'Что с погодой в Калининграде?',
            r'Сейчас в Калининграде',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая завтра погода в Сочи?',
            r'Завтра\b.*?\bв Сочи',
            [r'^На послезавтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая завтра погода в Апиа?',
            r'Завтра\b.*?\bв Апиа',
            [r'^На послезавтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая завтра погода в Алофи?',
            r'Завтра\b.*?\bв Алофи',
            [r'^На послезавтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Погода в Минске вечером',
            r'[вВ]ечером в Минске',
            [r'^На (после)?завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            f'Погода в Минске {get_day(delta=3)} вечером',
            r'[вВ]ечером в Минске',
            [r'^На сегодня$', r'^Что ты умеешь\?$'],
        ),

        # 2nd block (alice-1083)
        (
            'Какая сегодня погода',
            r'[сС]ейчас в ',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая сегодня температура',
            r'[сС]ейчас в ',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая сейчас погода',
            NOW_PHRASES + r' в ',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Тепло ли сейчас в Сочи?',
            NOW_PHRASES + r' в Сочи',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),

        # 3rd block (alice-415, alice-629)
        (  # and alice-1083
            'Какая погода вечером',
            r'[вВ]ечером в ',
            [r'^На (после)?завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая погода завтра утром?',
            r'[зЗ]автра утром в',
            [r'^На послезавтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Какая погода сегодня ночью',
            r'([сС]егодня ночью|[нН]очью) в',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Погода завтра вечером в Питере',
            r'[зЗ]автра вечером',
            [r'^На послезавтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
    ]

    @pytest.mark.parametrize('surface', [surface.navi, surface.station, surface.loudspeaker])
    @pytest.mark.parametrize('command, title_re, suggests_re', commands)
    def test_pogoda_common_simple(self, alice, command, title_re, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        assert re.search(title_re, response.text)

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        assert self._list_eq_re(suggests, suggests_re)

    @pytest.mark.parametrize('surface', [surface.searchapp, surface.launcher])
    @pytest.mark.parametrize('command, title_re, suggests_re', commands)
    def test_pogoda_common_searchapp(self, alice, command, title_re, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        assert not response.has_voice_response()

        # Проверка карточки
        weather_card = WeatherDivCard(response.div_card)
        assert re.search(title_re, weather_card.title)
        assert 'Ощущается как' in weather_card.subtitle

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        suggests_re = self._build_searchapp_suggests_re(suggests_re, command, search_suggest_is_last=False)
        assert self._list_eq_re(suggests, suggests_re)


class TestPalmPogodaInRange(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-619
    https://testpalm.yandex-team.ru/testcase/alice-623
    https://testpalm.yandex-team.ru/testcase/alice-635

    Частично https://testpalm.yandex-team.ru/testcase/alice-1507
    """

    commands = [
        (
            'Погода на три дня',
            r'^Погода в Москве на 3 дня',
            [get_day(delta=delta) for delta in range(3)]
        ),
        (
            'Погода на две недели',
            r'^Погода в Москве',
            [get_day(delta=delta) for delta in range(10)]
        ),
        (
            'Прогноз погоды на выходные',
            r'^Погода в Москве на выходные',
            None  # don't know holidays forward
        ),
        (
            'Погода в эти выходные',
            r'^Погода в Москве на выходные',
            None  # don't know holidays forward
        ),
        (
            'Погода в следующие выходные',
            r'^Погода в Москве на выходные',
            None  # don't know holidays forward
        ),
    ]

    @pytest.mark.parametrize('surface', [surface.station, surface.navi])
    @pytest.mark.parametrize('command, title_re, days', commands)
    def test_pogoda_in_range_simple(self, alice, command, title_re, days):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка дней в ответе, если возможно
        assert response.text
        if days:
            if len(days) == 10:
                weekday = alice.datetime_now.weekday()
                if weekday == 6:  # в воскресенье показываем 8 дней вместо 10
                    days = days[:-2]
                elif weekday == 5:  # в субботу показываем 9 дней вместо 10
                    days = days[:-1]

            for index, day in enumerate(days):
                if index == 0:
                    assert re.search(r'[сС]егодня', response.text)
                elif index == 1:
                    assert re.search(r'[зЗ]автра', response.text)
                elif index == 2:
                    assert re.search(r'[пП]ослезавтра', response.text)
                else:
                    assert day in response.text

    @pytest.mark.parametrize('surface', [surface.searchapp, surface.launcher])
    @pytest.mark.parametrize('command, title_re, days', commands)
    def test_pogoda_in_range_searchapp(self, alice, command, title_re, days):
        weekday = alice.datetime_now.weekday()
        if weekday in [5, 6] and command in ['Прогноз погоды на выходные', 'Погода в эти выходные']:
            return

        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        if response.has_voice_response():
            assert re.search(title_re, response.output_speech_text)

        # Проверка карточки
        weather_card = WeatherDivCard(response.div_card)
        assert re.search(title_re, weather_card.title)

        if weekday not in [5, 6]:
            assert weather_card.footer.action_url
            assert 'СМОТРЕТЬ ПРОГНОЗ НА МЕСЯЦ' in weather_card.footer.text

        # Проверка дней в карточке, если возможно
        if days:
            if len(days) == 10:
                if weekday == 6:  # в воскресенье показываем 8 дней вместо 10
                    days = days[:-2]
                elif weekday == 5:  # в субботу показываем 9 дней вместо 10
                    days = days[:-1]

            assert len(weather_card.days) == len(days)
            for index, day in enumerate(days):
                assert day in weather_card.days[index]

        assert response.suggests


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.station,
])
class TestPalmPogodaInCityNoData(PogodaDialogBase):
    """
    Part of TestPalmPogodaInCity (alice-582, alice-626, alice-636)
    """

    commands = [
        (
            f'Погода в Минске {get_day(delta=-3)} вечером',
            r'^Нет данных о погоде в Минске на это число.$',
            ['На сегодня', 'На завтра'],
            [],
        ),
        # (
        #     'Погода в Атлантиде',
        #     r', где это "в [аА]тлантиде".$',
        #     [],
        #     ['На завтра', 'На выходные', 'На послезавтра', 'На сегодня'],
        # ),
        # XFail: Reason=https://st.yandex-team.ru/HOLLYWOOD-1024
    ]

    @pytest.mark.parametrize('command, text_re, suggests_re, no_suggests_re', commands)
    def test(self, alice, command, text_re, suggests_re, no_suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        assert re.search(text_re, response.text)
        assert response.has_voice_response()
        assert response.text in response.output_speech_text

        # Проверка карточки
        assert not response.div_card

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        if surface.is_searchapp(alice) or surface.is_launcher(alice):
            suggests_re = self._build_searchapp_suggests_re(suggests_re, command)
        assert self._list_eq_re(suggests, suggests_re, ignore_case=True)
        for s_re in no_suggests_re:
            assert s_re not in suggests


@pytest.mark.voice
class TestPalmPogodaNoParams(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-414
    https://testpalm.yandex-team.ru/testcase/alice-630
    https://testpalm.yandex-team.ru/testcase/alice-631
    """

    commands = [
        (
            'Погода',
            r'[сС]ейчас в ',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Прогноз',
            NOW_PHRASES + r' в ',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Расскажи про погоду, пожалуйста',
            r'[сС]ейчас в ',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
    ]

    @pytest.mark.parametrize('surface', [surface.navi, surface.station, surface.loudspeaker])
    @pytest.mark.parametrize('command, title_re, suggests_re', commands)
    def test_pogoda_no_params_simple(self, alice, command, title_re, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        assert re.search(title_re, response.text)

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        assert self._list_eq_re(suggests, suggests_re)

    @pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp])
    @pytest.mark.parametrize('command, title_re, suggests_re', commands)
    def test_pogoda_no_params_searchapp(self, alice, command, title_re, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        if response.has_voice_response():
            assert re.search(title_re, response.output_speech_text)

        # Проверка карточки
        weather_card = WeatherDivCard(response.div_card)
        assert re.search(title_re, weather_card.title)
        assert 'Ощущается как' in weather_card.subtitle

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        suggests_re = self._build_searchapp_suggests_re(suggests_re, command, search_suggest_is_last=False)
        assert self._list_eq_re(suggests, suggests_re, ignore_case=True)


@pytest.mark.voice
class TestPalmPogodaForTheDay(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-581
    https://testpalm.yandex-team.ru/testcase/alice-625
    https://testpalm.yandex-team.ru/testcase/alice-634
    """

    commands = [
        (
            'Скажи погоду на сегодня',
            r'[сС]ейчас в Москве',
            [r'^На завтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Завтра будет дождь?',
            r'[зЗ]автра (днём |)в Москве',
            [r'^На послезавтра$', r'^На выходные$', r'^Что ты умеешь\?$'],
        ),
        (
            'Погода {}'.format(get_day(5)),
            r'{} (днём |)в Москве'.format(get_day(5)),
            [r'^На сегодня$', r'^На завтра$', r'^Что ты умеешь\?$'],
        ),
        (
            'Погода {}'.format(get_weekday(3)),
            r'{} (днём |)в Москве'.format(get_day(3)),
            [r'^На сегодня$', r'^На завтра$', r'^Что ты умеешь\?$'],
        ),
    ]

    @pytest.mark.parametrize('surface', [surface.navi, surface.station, surface.loudspeaker])
    @pytest.mark.parametrize('command, title_re, suggests_re', commands)
    def test_pogoda_for_the_day_simple(self, alice, command, title_re, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        assert re.search(title_re, response.text)

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        assert self._list_eq_re(suggests, suggests_re)

    @pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp])
    @pytest.mark.parametrize('command, title_re, suggests_re', commands)
    def test_pogoda_for_the_day_searchapp(self, alice, command, title_re, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text
        assert response.has_voice_response()
        assert re.search(title_re, response.output_speech_text)

        # Проверка карточки
        weather_card = WeatherDivCard(response.div_card)
        assert re.search(title_re, weather_card.title)
        assert 'Ощущается как' in weather_card.subtitle

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        suggests_re = self._build_searchapp_suggests_re(suggests_re, command, search_suggest_is_last=False)
        assert self._list_eq_re(suggests, suggests_re, ignore_case=True)


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.loudspeaker,
    surface.navi,
    surface.searchapp,
    surface.station,
])
class TestPalmPogodaForTheDayNoData(PogodaDialogBase):
    """
        Continuation for TestPalmPogodaForTheDay
    """

    commands = [
        (
            f'Погода {get_day(15)}',
            [r'^На сегодня$', r'^На завтра$'],
        ),
        (
            'Какая вчера была погода?',
            [r'^На сегодня$', r'^На завтра$'],
        ),
    ]

    @pytest.mark.parametrize('command, suggests_re', commands)
    def test(self, alice, command, suggests_re):
        response = alice(command)
        assert response.intent == intent.GetWeather

        # Проверка ответа
        assert response.text == 'Нет данных о погоде в Москве на это число.'
        assert response.has_voice_response()
        assert response.text in response.output_speech_text

        # Проверка карточки
        assert not response.div_card

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        if surface.is_searchapp(alice) or surface.is_launcher(alice):
            suggests_re = self._build_searchapp_suggests_re(suggests_re, command)
        assert self._list_eq_re(suggests, suggests_re, ignore_case=True)


@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.searchapp,
])
class TestPalmPogodaForTheDayNoDataText(PogodaDialogBase):
    """
        https://testpalm.yandex-team.ru/testcase/alice-2076
    """

    @pytest.mark.parametrize('command, place', [
        (f'Погода {get_day(delta=14)}', 'в Москве'),
        (f'Прогноз погоды в Лондоне в начале {get_month(delta=2)}', 'в Лондоне'),
    ])
    def test(self, alice, command, place):
        response = alice(command)
        assert response.intent == intent.GetWeather
        assert response.text == f'Нет данных о погоде {place} на это число.'


@pytest.mark.voice
class TestPalmPogodaForTheDayMulticommand(PogodaDialogBase):
    """
        Continuation for TestPalmPogodaForTheDay
    """

    commands = [
        (
            ['Завтра будет дождь?', 'На послезавтра'],
            r'([пП]огода |)[вВ] (Москве|Сасове)',
            [r'^На сегодня$', r'^На завтра$', r'^Что ты умеешь\?$'],
        ),
    ]

    @pytest.mark.parametrize('surface', [surface.navi, surface.station, surface.loudspeaker])
    @pytest.mark.parametrize('commands, title_re, suggests_re', commands)
    def test_pogoda_for_the_day_multicommand_simple(self, alice, commands, title_re, suggests_re):
        for command in commands:
            response = alice(command)

        assert response.intent == intent.GetWeatherEllipsis

        # Проверка ответа
        assert response.text
        assert re.search(title_re, response.text)

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        assert self._list_eq_re(suggests, suggests_re)

    @pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp])
    @pytest.mark.parametrize('commands, title_re, suggests_re', commands)
    def test_pogoda_for_the_day_multicommand_searchapp(self, alice, commands, title_re, suggests_re):
        for command in commands:
            response = alice(command)

        assert response.intent == intent.GetWeatherEllipsis

        # Проверка ответа
        assert response.text
        if response.has_voice_response():
            assert re.search(title_re, response.output_speech_text)

        # Проверка карточки
        weather_card = WeatherDivCard(response.div_card)
        assert re.search(title_re, weather_card.title)
        assert 'Ощущается как' in weather_card.subtitle

        # Проверка саджестов
        suggests = [s.title for s in response.suggests]
        with_search_suggest = self._build_searchapp_suggests_re(suggests_re, command=commands[-1], search_suggest_is_last=False)
        without_search_suggest = self._build_searchapp_suggests_re(suggests_re)
        assert self._list_eq_re(suggests, with_search_suggest) or self._list_eq_re(suggests, without_search_suggest)


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.navi,
    surface.searchapp,
])
class TestPalmPogodaDetails(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-632
    https://testpalm.yandex-team.ru/testcase/alice-1437
    """

    details_text_re = r'(А теперь о погоде в (подробностях|деталях)\.|Все, что вы хотели знать о погоде, но боялись спросить\.|Открываю (более |)подробную информацию о погоде\.)'

    @pytest.mark.parametrize('in_more_detail_command', [
        'Подробнее',
        'Больше деталей',
        # 'А какой ветер',  # может моргать
        # 'Поконкретнее',  # может моргать
    ])
    def test(self, alice, in_more_detail_command):
        response = alice('Погода')
        response = alice(in_more_detail_command)
        assert response.intent == intent.GetWeatherDetails

        # Проверка открытия страницы погоды
        assert response.directive
        assert response.directive.name == directives.names.OpenUriDirective
        assert 'pogoda' in response.directive.payload.uri

        # Проверка ответа
        assert response.text
        assert re.search(self.details_text_re, response.text)
        assert response.has_voice_response()
        assert re.search(self.details_text_re, response.output_speech_text)

        # Проверка саджестов
        if surface.is_searchapp(alice) or surface.is_launcher(alice):
            suggests = [s.title for s in response.suggests]
            assert self._list_eq_re(suggests, [r'^👍$', r'^👎$'])


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmPogodaIrrelevant(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2393
    """

    @pytest.mark.parametrize('command', [
        'температура короны солнца',
        'температура в атмосфере солнца по цельсию',
    ])
    def test(self, alice, command):
        response = alice(command)
        assert response.intent in [intent.Search, intent.Factoid, intent.ObjectAnswer]


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmPogodaTapOnCard(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-641
    """

    def test(self, alice):
        response = alice(f'погода {get_day(delta=2)}')
        assert response.intent == intent.GetWeather

        weather_card = WeatherDivCard(response.div_card)
        assert 'https://yandex.ru/pogoda' in unquote(weather_card.action_url)
        assert 'Утро' in weather_card.day_time[0]
        assert 'День' in weather_card.day_time[1]
        assert 'Вечер' in weather_card.day_time[2]
        assert 'Ночь' in weather_card.day_time[3]


@pytest.mark.version(hollywood=211)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmPogodaSearchSuggest(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-642
    """

    def test(self, alice):
        response = alice('погода')
        assert response.intent == intent.GetWeather

        search_suggest = response.suggest('🔍 "погода"')
        assert search_suggest
        response = alice.click(search_suggest)

        expected_text = 'Яндекс.Погода — прогноз погоды на 10 дней'

        assert response.text == expected_text
        assert len(response.directives) == 2

        assert response.directives[0].name == directives.names.FillCloudUiDirective
        assert response.directives[0].payload.text == expected_text

        assert response.directives[1].name == directives.names.OpenUriDirective
        assert 'https://yandex.ru/pogoda' in unquote(response.directives[1].payload.uri)
        assert ['Открыть', 'Поискать в Яндексе'] == [button.title for button in response.buttons]


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.loudspeaker(is_tv_plugged_in=False),
])
class TestPalmPogodaScreenOff(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-669
    """

    def test(self, alice):
        response = alice('Расскажи про погоду, пожалуйста')
        assert response.intent == intent.GetWeather

        assert response.has_voice_response()
        assert 'Сейчас в Москве' in response.output_speech_text


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmPogodaAutomaticGeo(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1342
    """

    @pytest.mark.region(region.Moscow)
    def test_pogoda_automatic_geo_moscow(self, alice):
        response = alice('Погода')
        assert response.intent == intent.GetWeather
        weather_card = WeatherDivCard(response.div_card)
        assert 'Сейчас в Москве' in weather_card.title

    @pytest.mark.region(region.Minsk)
    def test_pogoda_automatic_geo_minsk(self, alice):
        response = alice('Погода')
        assert response.intent == intent.GetWeather
        weather_card = WeatherDivCard(response.div_card)
        assert 'Сейчас в Минске' in weather_card.title

    @pytest.mark.region(region.Berlin)
    def test_pogoda_automatic_geo_berlin(self, alice):
        response = alice('Погода')
        assert response.intent == intent.GetWeather
        weather_card = WeatherDivCard(response.div_card)
        assert 'Сейчас в Берлине' in weather_card.title


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmPogodaWatch(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-17
    """

    @pytest.mark.region(region.Moscow)
    @pytest.mark.xfail(reason="https://st.yandex-team.ru/HOLLYWOOD-1029")
    def test_alice_17(self, alice):
        response = alice('Где я?')
        assert response.intent == intent.GetMyLocation
        assert 'Москва' in response.text

        response = alice('Какая погода?')
        assert response.intent == intent.GetWeather
        assert re.search(NOW_PHRASES + r' в Москве', response.text)

        response = alice('А во Владивостоке?')
        assert response.intent == intent.GetWeatherEllipsis
        assert re.search(NOW_PHRASES + r' во Владивостоке', response.text)

        response = alice('Какая погода в Атлантиде?')
        assert response.intent == intent.GetWeather
        self._assert_unknown_where(response.text, 'в Атлантиде')

        response = alice('Будет дождь в выходные?')
        if alice.datetime_now.weekday() == 6:  # Monday - Friday
            assert response.intent == intent.GetWeatherNowcast
        else:  # Saturday - Sunday
            assert response.intent == intent.GetWeather
            assert re.search(r'[вВ] Москве', response.text)

        response = alice('Какая погода будет в Петербурге через неделю?')
        assert response.intent in [intent.GetWeather, intent.GetWeatherEllipsis]
        assert re.search(r'[вВ] Санкт-Петербурге', response.text)


@pytest.mark.parametrize('surface', [surface.old_automotive])
class TestPalmPogodaOldAutomotive(PogodaDialogBase):
    """
    Частично https://testpalm.yandex-team.ru/testcase/alice-1800
    Частично https://testpalm.yandex-team.ru/testcase/alice-1802
    """

    @pytest.mark.region(region.Moscow)
    @pytest.mark.parametrize('command', ['погода', 'открой погоду'])
    def test(self, alice, command):
        response = alice(command)
        assert response.intent == intent.GetWeather
        assert 'Сейчас в Москве' in response.text


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmPogodaDayPart(PogodaDialogBase):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1973
    https://testpalm.yandex-team.ru/testcase/alice-1974
    """

    @pytest.mark.parametrize('when, title', [
        ('сегодня', 'Вечером в Ростове-на-Дону'),
        ('послезавтра', 'Послезавтра вечером в Ростове-на-Дону'),
        (get_day(delta=5), f'{get_day(delta=5)} вечером в Ростове-на-Дону'),
    ])
    def test(self, alice, when, title):
        response = alice(f'погода вечером в ростове {when}')
        assert response.intent == intent.GetWeather

        # Проверка карточки
        weather_card = WeatherDivCard(response.div_card)
        url_prefix = 'https://yandex.ru/pogoda?from=alice_weathercard&lat=47.222078&lon=39.720349&utm_campaign=card&utm_medium=forecast&utm_source=alice'
        assert weather_card.data.action_url.startswith(url_prefix)
        assert title in weather_card.title
