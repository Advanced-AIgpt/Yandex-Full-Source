import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['get_date']

EXPERIMENTS_GENERATIVE = [
    'bg_fresh_granet_form=personal_assistant.scenarios.get_date'
]


@pytest.mark.scenario(name='GetDate', handle='get_date')
@pytest.mark.experiments(*EXPERIMENTS_GENERATIVE)
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class Tests:

    @pytest.mark.parametrize('command, expected', [
        pytest.param('какое сегодня число', '20 января', id='day_today'),
        pytest.param('какая дата будет вторник', '21 января', id='day_in2w'),
        pytest.param('какая дата была 9 мая 1945 года', '9 мая', id='day_epoch'),  # Check for -1 day for dates before EPOCH
        pytest.param('какой день недели был 9 мая 1945 года', 'среда', id='dayw_epoch'),
        pytest.param('какой день недели 1 октября 2021 в Москве', 'пятница', id='dayweek_20211001'),
        pytest.param('Сегодня в Токио понедельник или вторник?', 'понедельник', id='monday_or1'),
        pytest.param('Сегодня в Токио понедельник или вторник?', 'токио', id='monday_or2'),
        # pytest.param('Какой день будет 13 октября через пять лет?', 'понедельник', id='after5year'),
        pytest.param('какой сегодня день месяца', 'понедельник', id='dayofmonth'),
        pytest.param('вторник тридцать первое число', '21 января', id='question1'),
        pytest.param('сегодня пятница тринадцатое число', 'понедельник', id='question2'),
        pytest.param('алиса какой день недели будет первого сентября две тысячи двадцать первого года', 'среда', id='day010921'),
        pytest.param('алиса сегодня двадцать восьмое число', 'понедельник', id='question3'),
        pytest.param('тридцатое августа две тысячи четырнадцатый год какой день недели', 'суббота', id='dayinpast'),
        pytest.param('а сегодня а сейчас какой день недели и какое число', 'сегодня', id='daytimeoffset'),
    ])
    def test_make_get_date(self, alice, command, expected):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run'}
        assert expected in r.run_response.ResponseBody.Layout.OutputSpeech.lower()
