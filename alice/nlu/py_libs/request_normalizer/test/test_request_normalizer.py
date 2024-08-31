from alice.nlu.py_libs.request_normalizer.lang import Lang
from alice.nlu.py_libs.request_normalizer.request_normalizer import RequestNormalizer
import pytest


@pytest.mark.parametrize('input_text, expected_result', [
    ('сто к одному', '100 к 1'),
    ('попроси розового слона принести швабры', 'попроси розового слона принести швабры'),
    ('первый номер', '1 номер'),
    ('трижды финалист', 'трижды финалист'),
    ('', ''),
])
def test_text_normalized_correctly(input_text, expected_result):
    actual_result = RequestNormalizer.normalize(Lang.RUS, input_text)
    assert actual_result == expected_result


@pytest.mark.parametrize('input_text, lang', [
    (None, Lang.RUS),
    ('привет', None),
])
def test_error_raised(input_text, lang):
    with pytest.raises(TypeError):
        RequestNormalizer.normalize(lang, input_text)
