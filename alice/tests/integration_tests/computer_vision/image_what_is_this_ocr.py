import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.div_card as div_card
import computer_vision.util as util


def _assert_read_text_response(response):
    assert response.intent == intent.ImageOcrVoice
    assert '! ATTENTION! CHECK OIL & FUEL LEVELS CHECK OIL LEVEL' in response.text
    assert response.has_voice_response()
    assert '! ATTENTION!.sil<[300]> CHECK OIL & FUEL LEVELS' in response.output_speech_text


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestImageWhatIsThisOcr(object):

    owners = ('polushkin', 'g:cv-search', )

    photo = 'http://avatars.mds.yandex.net/get-images-similar-mturk/40560/588abc3b0369f792738f9074d115d832/orig'

    def test_ocr(self, alice):
        response = alice.search_by_photo(self.photo)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.intent == intent.ImageOcr
        assert response.text in [
            'Так. Это текст.',
            'Вижу текст — распознаю.',
            'Это текст, точно вам говорю!',
            'Сейчас скажу, что тут написано.',
            'Сейчас разберёмся, что тут написано.',
            'Тут что-то написано. Секундочку...',
        ]
        assert response.has_voice_response()

        assert len(response.buttons) == 2
        find_and_translate_button = response.button('Найти и перевести текст')
        assert find_and_translate_button
        assert find_and_translate_button.directives[0].payload.uri

        read_button = response.button('Зачитать текст')
        assert read_button
        read_button_response = alice.click(read_button)
        _assert_read_text_response(read_button_response)

        # Return to previous state
        assert read_button_response.suggest('Распознанный текст')
        assert alice.click(read_button_response.suggest('Распознанный текст'))

        expected_more_info_buttons = {
            'Озвучить': intent.ImageOcrVoice,
            'Что это?': intent.ImageSimilar,
            **util.common_more_info_buttons
        }
        more_info_card = div_card.MoreInfo(response.div_card)
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, expected_more_info_buttons.keys())
        for title, expected_intent in expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        util.assert_suggests(response.suggests, {'Зачитай этот текст', 'Что это?'})

    def test_ocr_voice(self, alice):
        response = alice.read_text(self.photo)
        assert response.scenario == scenario.ImageWhatIsThis
        _assert_read_text_response(response)

        assert len(response.buttons) == 3
        read_next_button = response.button('Прочитать следующий текст')
        assert read_next_button
        read_next_response = alice.click(read_next_button)
        util.assert_start_image_recogizer(read_next_response.directive, mode_name='voice_text', camera_type=None)

        read_again_button = response.button('Прочитать еще раз')
        assert read_again_button
        _assert_read_text_response(alice.click(read_again_button))

        ocr_text_button = response.button('Распознанный текст')
        assert ocr_text_button.directives[0].payload.uri

        expected_more_info_buttons = {
            'Текст': intent.ImageOcr,
            'Что это?': intent.ImageSimilar,
            **util.common_more_info_buttons
        }
        more_info_card = div_card.MoreInfo(response.div_card)
        assert 'Ещё информация по этой картинке' in more_info_card.title
        util.assert_div_card_buttons(more_info_card.buttons, expected_more_info_buttons.keys())
        for title, expected_intent in expected_more_info_buttons.items():
            button_response = alice.click(more_info_card.button(title).image)
            assert button_response.intent == expected_intent

        util.assert_suggests(response.suggests, {'Распознанный текст', 'Что это?'})
