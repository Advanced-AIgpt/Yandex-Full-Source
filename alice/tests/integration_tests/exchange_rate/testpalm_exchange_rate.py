import re

import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


# matches '69 рублей 14 копеек.\nМосковская биржа, 4 июня в 17:58.'
# matches '673607 рублей.\nCoindesk.com, 4 июня в 15:26.'
# matches '77 рублей 32 копейки.\nЦБ РФ, 5 июня.'
# matches '69 рублей 14 копеек.\nМосковская биржа, 4 июня в 10:58 утра.'
# kopecks and daytime are optional
RUBLE_COURSE_RESPONSE = r'^(?P<ruble>\d+ рубл[а-я]+) ?(?P<kopeck>\d+ копе[а-я]+)?.+\n' \
                        r'(?P<source>.+), (?P<day>\d+) (?P<month>.*?)( в (?P<hour>\d{1,2}):(?P<minute>\d{1,2}).*?)?\.$'

# matches '1 евро 28 евроцентов.\nПо курсу Московской биржи на 19:48 4 июня.'
# matches '1 евро 28 евроцентов.\nПо курсу ЦБ РФ на 6 июня.'
# matches '1 евро 28 евроцентов.\nПо курсу Московской биржи на 11:30 утра 23 июня.'
# matches '97 евроцентов.\nПо курсу ЦБ РФ на 1 марта.'
EURO_COURSE_RESPONSE = r'^(?P<ruble>\d+ евро)? ?(?P<kopeck>\d+ евроцент[а-я]*)?\.\n' \
                       r'По курсу (?P<source>.+) на ((?P<hour>\d{1,2}):(?P<minute>\d{1,2}) .*?)?(?P<day>\d+) (?P<month>.*?)\.$'

# matches '7220 рублей 16 копеек.\nПо курсу Московской биржи на 19:32 4 июня.'
# matches '7172 рубля 85 копеек.\nПо курсу ЦБ РФ на 6 июня.'
# matches '7144 рубля 3 копейки.\nПо курсу Московской биржи на 11:30 утра 23 июня.'
RUBLE_AMOUNT_RESPONSE = r'^(?P<ruble>\d+ рубл[а-я]+) ?(?P<kopeck>\d+ копе[а-я]+)?\.\n' \
                        r'По курсу (?P<source>.+) на ((?P<hour>\d{1,2}):(?P<minute>\d{1,2}) .*?)?(?P<day>\d+) (?P<month>.*?)\.$'

# matches '14 октября в 17:13 курс Московской биржи составлял 82 рубля 77 копеек за 1 евро.'
# matches '14 октября в 17:13 курс Московской биржи – 82 рубля 77 копеек за 1 евро.'
# matches '15 октября курс ЦБ РФ равен 83 рублям 33 копейкам за 1 евро.'
# matches '9 марта курс ЦБ РФ равен 122 рублям за 1 евро.'
RUBLE_COURSE_WAS_RESPONSE = r'(?P<day>\d+) (?P<month>.*?)( в (?P<hour>\d{1,2}):(?P<minute>\d{1,2}))? ' \
                            r'курс .* (?P<ruble>\d+ рубл[а-я]+) ?(?P<kopeck>\d+ копе[а-я]+)? за 1 евро\.$'


@pytest.mark.voice
@pytest.mark.parametrize('surface', surface.actual_surfaces + [surface.old_automotive])
class TestPalmExchangeRate(object):
    """
    Логическая часть следующих тестов:
    https://testpalm.yandex-team.ru/testcase/alice-10
    https://testpalm.yandex-team.ru/testcase/alice-498
    https://testpalm.yandex-team.ru/testcase/alice-1092
    https://testpalm.yandex-team.ru/testcase/alice-1332
    https://testpalm.yandex-team.ru/testcase/alice-1506
    https://testpalm.yandex-team.ru/testcase/alice-1548
    https://testpalm.yandex-team.ru/testcase/alice-2164
    Частично https://testpalm.yandex-team.ru/testcase/alice-1802
    """

    owners = ('sparkle',)

    @pytest.mark.parametrize('command, template', [
        ('какой курс доллара?', RUBLE_COURSE_RESPONSE),
        ('какой сегодня курс доллара?', RUBLE_COURSE_RESPONSE),
        ('курс биткоина', RUBLE_COURSE_RESPONSE),
        ('курс доллара', RUBLE_COURSE_RESPONSE),
        ('курс евро на завтра', RUBLE_COURSE_RESPONSE),
        ('курс евро', RUBLE_COURSE_RESPONSE),
        ('курс фунта стерлинга на завтра', RUBLE_COURSE_RESPONSE),
        ('сто рублей это сколько в евро', EURO_COURSE_RESPONSE),
        ('92 евро это сколько в рублях', RUBLE_AMOUNT_RESPONSE),
    ])
    def test_ruble_course(self, alice, command, template):
        response = alice(command)
        assert response.intent == intent.Convert
        assert response.text
        assert response.has_voice_response()

        matches = re.search(template, response.text)
        assert matches, f'No match in response "{response.text}"'

        # ('50 рублей', '19 копеек') or ('50 евро', '19 евроцентов')
        ruble, kopeck = matches['ruble'] or '', matches['kopeck'] or ''
        assert ruble or kopeck, 'Must be either banknotes or coins or both'
        assert ruble in response.output_speech_text and kopeck in response.output_speech_text

    def test_too_far_ruble_course(self, alice):
        response = alice('курс евро на послезавтра')
        assert response.intent == intent.Convert
        assert response.text
        assert response.has_voice_response()

        fail_text = ['неизвестен', 'у меня нет информации', 'я не знаю курса на']

        # В субботу и вечером пятницы мы можем знать курс на понедельник
        is_saturday = alice.datetime_now.weekday() in [4, 5]
        if is_saturday and all(dont_know not in response.text for dont_know in fail_text):
            template = RUBLE_COURSE_RESPONSE
        else:
            template = RUBLE_COURSE_WAS_RESPONSE
            for text in [response.text, response.output_speech_text]:
                assert any(dont_know in text for dont_know in fail_text), f'No match in response "{text}"'

        matches = re.search(template, response.text)
        assert matches, f'No match in response "{response.text}"'
        ruble, kopeck = matches['ruble'], matches['kopeck'] or ''
        assert ruble in response.output_speech_text and kopeck in response.output_speech_text


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.launcher,
    surface.loudspeaker,
    pytest.param(
        surface.navi,
        marks=pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-1286'),
    ),
    surface.searchapp,
    surface.station,
    surface.watch,
    surface.yabro_win,
])
class TestPalmStockPrice(object):
    """
    Логическая часть следующих тестов:
    https://testpalm.yandex-team.ru/testcase/alice-10
    https://testpalm.yandex-team.ru/testcase/alice-498
    https://testpalm.yandex-team.ru/testcase/alice-1092
    https://testpalm.yandex-team.ru/testcase/alice-1332
    https://testpalm.yandex-team.ru/testcase/alice-1506
    https://testpalm.yandex-team.ru/testcase/alice-1548
    https://testpalm.yandex-team.ru/testcase/alice-2164
    """

    owners = ('sparkle', 'zhigan')

    @pytest.mark.parametrize('command, template', [
        ('курс акций яндекса', r'Курс акций компании Яндекс.* состав(ил|ляет) \d+ доллар.*( \d+ цент)?.*'),
        ('сколько стоит нефть', r'Цена\' на нефть .* составила \d+ доллар.*( \d+ цент)?.* за баррель'),
    ])
    def test_stock_price(self, alice, command, template):
        response = alice(command)
        assert response.intent in [intent.Search, intent.Factoid]
        assert response.has_voice_response()
        assert re.search(template, response.output_speech_text)
