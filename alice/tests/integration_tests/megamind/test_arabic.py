import re

import alice.tests.library.locale as locale
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


def assert_is_arabic_text(text):
    assert not re.search('[а-я]', text, re.IGNORECASE), 'В ответе не должно быть русских букв'
    assert re.search('[\u0621-\u064A\u0660-\u0669]', text, re.IGNORECASE), 'В ответе должны присутствовать арабские символы'


@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.parametrize('locale', [locale.ar_sa(use_tanker=False)])
class TestArabic(object):
    """
    Тестируем, что арабские запросы не падают, а в ответе содержится арабский текст, отсутствует русский текст.
    Список сценариев выбран так, чтобы протестировать нативно арабские сценарии, переводные русские сценарии, винс.
    """

    owners = ('alexanderplat', 'abc:megamind')

    @pytest.mark.parametrize('command, scenario_name', [
        pytest.param('تغيير مستوى الصوت إلى 1234', scenario.Commands, id='commands'),  # Измените громкость на 1234
        pytest.param(
            'ما هو اسمك', scenario.GeneralConversation, id='general_conversation_microintent',  # Как тебя зовут
            marks=pytest.mark.xfail(reason='Vins answers with microintents')
        ),
        pytest.param('ما هو الجديد أن أقول', scenario.GeneralConversation, id='general_conversation'),  # Что новенького расскажешь
        pytest.param('اضبط المنبه على الساعة 7 صباحا', scenario.Alarm, id='alarm'),  # Установите будильник на 7 утра
        pytest.param('الطقس', scenario.Weather, id='weather'),  # Погода
        pytest.param('أرني الأخبار', scenario.News, id='news'),  # Покажи мне новости
        pytest.param(
            'تحويل 10 يورو إلى دولار', scenario.Vins, id='vins_converter',  # Конвертировать 10 евро в доллары
            marks=pytest.mark.xfail(reason='Facts scenario answers')),
        pytest.param('قم بتشغيل التلفزيون', scenario.Vins, id='vins_tv'),  # Включи телевизор
    ])
    def test_scenario_response(self, alice, command, scenario_name):
        response = alice(command)
        assert response.scenario == scenario_name
        assert_is_arabic_text(response.text)

    @pytest.mark.experiments('mm_scenario=InvalidScenario')
    def test_error_response(self, alice):
        # Проверяем сообщение об ошибке, которое рендерится в MegaMind
        response = alice('привет')
        assert response.scenario is None
        assert_is_arabic_text(response.text)

    @pytest.mark.experiments(f'mm_scenario={scenario.DoNothing}')
    def test_empty_text_and_voice(self, alice):
        response = alice('لا تفعل أي شيء')  # Ничего не делай
        assert response.scenario == scenario.DoNothing
        assert not response.text and not response.has_voice_response() and not response.output_speech_text, \
               'Сценарий чем-то ответил, хотя мы хотели протестировать пустой ответ'

    @pytest.mark.version(hollywood=188, megamind=221)
    def test_conjugator_modifier(self, alice):
        response = alice('الطقس')  # погода
        assert response.scenario == scenario.Weather
        conjugator_modifier_analytics_info = response.modifiers_analytics_info.get('conjugator', {})
        assert conjugator_modifier_analytics_info.get('conjugated_phrases_count', 0) > 0 and \
            conjugator_modifier_analytics_info.get('is_output_speech_conjugated', False) and \
            conjugator_modifier_analytics_info.get('conjugated_cards_count', 0) > 0, \
            'Должен был отработать модификатор согласоватора, но почему-то он не заполнил analytics_info'
