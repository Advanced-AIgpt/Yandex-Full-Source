import alice.tests.library.directives as directives
import alice.tests.library.intent as intent


def _assert_equal(real, expected, text=''):
    assert real == expected, f'{text} {real} != {expected}'


_common_expected_suggests = {'👍', '👎', 'Похожие картинки', 'Информация о картинке', 'Что ты умеешь?'}


def assert_suggests(suggests, expected_suggests=set()):
    suggest_titles = {_.title for _ in suggests}
    expected = expected_suggests | _common_expected_suggests
    _assert_equal(suggest_titles, expected)


common_more_info_buttons = {
    'Похожие картинки': intent.ImageSimilarLike,
    'Поиск по картинке': intent.ImageInfo,
}


def assert_price(price):
    currencies = ('₽', '€', '$', '₴', 'тнг.', 'б.p.', 'TL')
    assert price, 'must has price'
    assert any((_ in price for _ in currencies)) or 'Нет в продаже' in price, 'Neither currency, neither not in stock in price'


def assert_div_card_buttons(buttons, expected_buttons):
    def _strip_font(title):
        return title[22:-7].strip('&nbsp;').rstrip('<br>')

    button_titles = {_strip_font(_.title) for _ in buttons}
    _assert_equal(button_titles, expected_buttons)


def assert_start_image_recogizer(directive, mode_name, camera_type):
    assert directive, 'must has directive'
    _assert_equal(directive.name, directives.names.StartImageRecognizerDirective)
    if mode_name:
        _assert_equal(directive.payload.image_search_mode_name, mode_name, 'Image recognizer mode mismatch')
    if camera_type:
        _assert_equal(directive.payload.camera_type, camera_type, 'Image recognizer camera type mismatch')
