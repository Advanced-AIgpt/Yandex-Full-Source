import pytest
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
from google.protobuf.json_format import MessageToJson

SCENARIO_NAME = 'Search'
SCENARIO_HANDLE = 'search'


@pytest.fixture(scope='module')
def enabled_scenarios():
    return [SCENARIO_HANDLE]


@pytest.mark.evo
@pytest.mark.voice
@pytest.mark.scenario(name=SCENARIO_NAME, handle=SCENARIO_HANDLE)
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.experiments('search_new_richcard_centaur')
class TestsCentaur:

    @pytest.mark.parametrize('command, answer', [
        pytest.param('Какого роста Эйнштейн', '175', id='einstein'),
        pytest.param('Кто написал книгу В поисках утраченного времени', 'пруст', id='book'),
        pytest.param('Сколько калорий в апельсине', '43', id='orange'),
        pytest.param('Столица Аргентины', 'буэнос-айрес', id='argentina'),
        pytest.param('В каком году родился Аркадий Волож', '1964', id='volozh'),
    ])
    def test_centaur_facts(self, alice, command, answer):
        response = alice(command)
        assert response.scenario == scenario.Search

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.ShowViewDirective
        assert answer in response.output_speech_text.lower()
        assert response.directives[1].name == directives.names.TtsPlayPlaceholderDirective

    @pytest.mark.parametrize('command', [
        pytest.param('какая калорийность у яблока', id='apple_calorific'),
        pytest.param('сколько будет 10 плюс 10', id='calculator_10'),
        pytest.param('сколько будет 10 поделить на 3', id='calculator_float'),
        pytest.param('кто такой джеф безос', id='suggest_fact'),
        pytest.param('сколько лет наполеону', id='entity_fact'),
        pytest.param('15 километров в мили', id='units_converter'),
        pytest.param('разница во времени между москвой и чикаго', id='time_difference'),
        pytest.param('расстояние между москвой и владивостоком', id='distance_fact'),
        pytest.param('льва толстого 16 индекс', id='zip_code'),
        pytest.param('счет в матче арсенала', id='sport_live_score'),
        pytest.param('москва', id='object_answer_as_fact')
    ])
    def test_centaur_questions(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search

        return MessageToJson(response.raw)
