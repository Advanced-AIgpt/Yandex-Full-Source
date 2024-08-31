import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['how_to_spell']


@pytest.mark.scenario(name='HowToSpell', handle='how_to_spell')
@pytest.mark.experiments('bg_fresh_granet', 'mm_enable_protocol_scenario=HowToSpell')
class Tests:
    @pytest.mark.parametrize('command,output_text', [
        pytest.param('как пишется слово молоко', 'Интересное слово. В нём три \"о\". Читаю по буквам: молоко.', id='dictionary-member'),
        pytest.param('как пишется слово предыстория', 'Читаю по буквам: предыстория.', id='default'),
        pytest.param('как пишется слово брошура', 'Читаю по буквам: брошюра.', id='mistake-in-asr-recognition-single-word'),
        pytest.param('как пишется слово через чур', 'Чересчур часто в этом слове допускают ошибки. '
                     'Запоминайте: слитно и с буквой "с". Читаю по буквам: чересчур.', id='mistake-in-asr-recognition-with-multiple-words'),
        pytest.param('как пишется слово молодежь', 'На конце "ж" с мягким знаком. Читаю по буквам: молодёжь.',
                     id='mistake-in-asr-recognition-single-yo-word'),
        pytest.param('как пишется все таки', 'Через дефис. Читаю по буквам: всё-таки.', id='mistake-in-asr-recognition-missing-dash-and-yo'),
        pytest.param('как пишется все', 'Читаю по буквам: все.', id='do-not-rewrite-e-word-to-yo-word-when-e-word-exists'),
        pytest.param('как пишется буква г',
                     'Такая палочка с носом, смотрящая направо. Можно в азбуке посмотреть.', id='letter',
                      marks=pytest.mark.xfail(reason='letters are disabled for now')),
        pytest.param('как пишется также',
                     'Если это союз, то пишем слитно. Пример: Алиса любит смотреть видео, а также сериалы. '
                     'Если наречие с частицей, то пишем раздельно. Алиса хочет чувствовать так же, как люди. '
                     'Ой, откровенный пример получился.', id='rule', marks=pytest.mark.xfail(reason='rules are disabled for now')),
        pytest.param('какое слово проверочное к слову молоко', 'Читаю по буквам: молоко.',
                     id='verification-word-not-in-dictionary', marks=pytest.mark.xfail(reason='verification-words-requests are disabled for now')),
        pytest.param('каким словом можно проверить слово скворец',
                     'Проверочное слово ласковое — скворушка. Читаю по буквам: скворушка.',
                      id='verification-word-in-dictionary', marks=pytest.mark.xfail(reason='verification-words-requests are disabled for now')),
    ])
    @pytest.mark.parametrize('surface', [surface.station])
    def test_how_to_spell(self, alice, command, output_text):
        response = alice(voice(command))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody

        assert len(response_body.Layout.Cards) > 0
        assert response_body.Layout.Cards[0].Text == output_text

    @pytest.mark.parametrize('surface', [surface.yabro_win, surface.smart_tv, surface.searchapp, surface.navi])
    def test_works_on_smart_speakers_only(self, alice):
        response = alice(voice('как пишется слово молоко'))
        assert response.scenario_stages() == {'run'}
        assert response.run_response.Features.IsIrrelevant
        assert len(response.run_response.ResponseBody.Layout.Cards) > 0
        assert response.run_response.ResponseBody.Layout.Cards[0].Text == 'Слишком сложно, не знаю таких правил.'
