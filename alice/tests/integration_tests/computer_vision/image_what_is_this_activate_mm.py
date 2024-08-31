import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import computer_vision.util as util


@pytest.mark.experiments(
    'vins_add_irrelevant_intents=personal_assistant.scenarios.image_what_is_this'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_barcode'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_clothes'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_frontal_similar_people'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_market'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_ocr'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_ocr_voice'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_office_lens'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_similar_artwork'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_similar_people'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_similar'
    '%2Cpersonal_assistant.scenarios.image_what_is_this_translate'
)
class TestImageWhatIsThisActivate(object):

    owners = ('polushkin', 'g:cv-search', )

    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, mode, camera_type', [
        ('что изображено на фото', 'smartcamera', None),
        ('найди похожую одежду на фото', 'smartcamera', None),
        ('на кого я похож', None, 'front'),
        ('найди похожий товар', 'smartcamera', None),
        ('прочти текст на картинке', 'voice_text', None),
        ('что здесь написано', 'text', None),
        ('сделай скан', 'doc_scanner', None),
        ('на какую картину это похоже', 'photo', None),
        ('на кого он похож', 'photo', None),
        ('включи умную камеру', 'smartcamera', None),
        ('переведи текст', 'translate', None),
    ])
    def test_common_activate(self, alice, command, mode, camera_type):
        response = alice(command)
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.product_scenario == 'images_what_is_this'
        assert response.text_card
        util.assert_start_image_recogizer(response.directive, mode, camera_type)

    @pytest.mark.parametrize('surface', [surface.station])
    def test_inability(self, alice):
        response = alice('что изображено на фото')
        assert response.scenario == scenario.ImageWhatIsThis
        assert response.text in [
            'Я еще не научилась этому. Давно собираюсь, но все времени нет.',
            'Я пока это не умею.',
            'Я еще не умею это.',
            'Я не могу пока, но скоро научусь.',
            'Меня пока не научили этому.',
            'Когда-нибудь я смогу это сделать, но не сейчас.',
            'Надеюсь, я скоро смогу это делать. Но пока нет.',
            'Я не знаю, как это сделать. Извините.',
            'Так делать я еще не умею.',
            'Программист Алексей обещал это вскоре запрограммировать. Но он мне много чего обещал.',
            'К сожалению, этого я пока не умею. Но я быстро учусь.',
        ]
