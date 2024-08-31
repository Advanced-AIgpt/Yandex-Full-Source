# coding: utf-8
from __future__ import unicode_literals

import math
import pytest

from vins_core.common.sample import Sample
from vins_core.dm.request import create_request
from vins_core.dm.form_filler.models import Form, Slot
from personal_assistant.item_selection import TfIdfItemMatcher, TVGalleryItemSelector


@pytest.fixture(scope='module')
def tv_selector():
    selector = TVGalleryItemSelector()
    return selector


@pytest.fixture(scope='module')
def tv_selector_with_synonyms():
    selector = TVGalleryItemSelector(synonym_files=[
        'personal_assistant/config/scenarios/entities/tv_channels.json'
    ])
    return selector


@pytest.mark.parametrize(('item1', 'item2', 'score'), [
    # full match
    ('собака', 'собака', pytest.approx(1.0)),
    ('собаке', 'собаке', pytest.approx(1.0)),

    # full match, multiple words
    ('собака кошка', 'собака кошка', pytest.approx(1.0)),
    ('собаки кошки', 'собаки кошки', pytest.approx(1.0)),

    # full match, multiple words, multiple parses for the second word
    ('собака улыбака', 'собака улыбака', pytest.approx(1.0)),
    ('собаки улыбаки', 'собаки улыбаки', pytest.approx(1.0)),

    # match by lemma
    ('собака', 'собаки', pytest.approx(0.5)),
    ('собаки', 'собаке', pytest.approx(0.5)),

    # Test that conjunctions, prepositions and particles are almost ignored (the result is almost 0.5 or almost 1)
    ('собака', 'на собаке', pytest.approx(1.0 / math.sqrt(2.0) / math.sqrt(2.02))),
    ('и собака', 'на собаке', pytest.approx(1.0 / math.sqrt(2.02) / math.sqrt(2.02))),
    ('собака', 'ах собака', pytest.approx(2.0 / math.sqrt(2.0) / math.sqrt(2.02))),
    ('ну собака', 'собака', pytest.approx(2.0 / math.sqrt(2.0) / math.sqrt(2.02))),
    ('ах', 'ах', pytest.approx(1.0)),
    ('на', 'в', 0.0),
    ('на', 'собака', 0.0),
    ('на', 'на собаках', pytest.approx(0.1 / math.sqrt(1.01))),

    # partial match, one parse for each word, word matches exactly
    ('собака кошка', 'собака мышка', pytest.approx(0.5)),

    # partial match, one parse for each word, match by lemma
    ('собака кошка', 'собаке мышка', pytest.approx(0.25)),
    ('собака кошка', 'собаке', pytest.approx(1.0 / 2.0 / math.sqrt(2))),  # about 0.35

    # partial match, one parse for each word, exact match, similarity is about 0.7
    ('собака кошка', 'собака', pytest.approx(1.0 / math.sqrt(2.0))),
    ('собаке кошка', 'собаке', pytest.approx(1.0 / math.sqrt(2.0))),
    ('собаке кошке', 'собаке', pytest.approx(1.0 / math.sqrt(2.0))),

    # partial match, 'улыбака' has multiple parses, word matches exactly, similarity is about 0.75
    ('собака улыбака', 'собака', pytest.approx(2.0 / (math.sqrt(2.0 * 0.5 * 0.5 + 3.0) * math.sqrt(2.0)))),

    # partial match, 'улыбака' has multiple parses, word matches by lemma, similarity is about 0.375
    ('собака улыбака', 'собаке', pytest.approx(1.0 / (math.sqrt(2.0 * 0.5 * 0.5 + 3.0) * math.sqrt(2.0)))),

    # numbers are very much like prepositions: they have reduced idf
    ('10', '10', pytest.approx(1.0)),
    ('терминатор 10', 'гладиатор 10', pytest.approx(0.5 ** 2 / (1.0 + 0.5 ** 2))),
    # номер is a helper word, and it has reduced idf
    ('1', 'номер 1', pytest.approx(0.5 / math.sqrt(0.25 ** 2 + 0.5 ** 2))),

    # match by alternate parse, about 0.33333
    # механик -> {"normal_form__механика": 0.5, "normal_form__механик": 0.5, "word__механик": 1.0}
    # механика -> {"normal_form__механик": 0.5, "normal_form__механика": 0.5, "word__механика": 1.0}
    ('механик', 'механика', pytest.approx(0.5 / 1.5)),

    # Empty string
    ('', '', pytest.approx(0.0)),
    ('собака', '', pytest.approx(0.0)),
    ('', 'собака', pytest.approx(0.0)),
])
def test_tf_idf_item_matcher(item1, item2, score):
    matcher = TfIdfItemMatcher(
        helper_words={'тв', 'tv', 'канал', 'channel', 'телеканал', 'телевидение', 'номер'},
        number_idf=0.5
    )
    assert matcher.get_score(item1, item2) == score


@pytest.mark.parametrize(('user_text', 'gallery_text', 'min_score', 'max_score'), [
    ('ру тв', 'ru tv', 0.6, 1.01),
    ('ru tv', 'ру тв', 0.6, 1.01),
    ('ру тв', 'муз тв', 0, 0.3),
    ('простоквашино тв', 'простоквашино 24', 0.6, 1.01),
    ('da vinci', 'DA VINCI LEARNING', 0.5, 0.75),
    pytest.param('да винчи', 'DA VINCI LEARNING', 0.5, 1.01, marks=pytest.mark.xfail(reason='imperfect transliteration')),
])
def test_translit_channel_selection(tv_selector, user_text, gallery_text, min_score, max_score):
    matcher = TfIdfItemMatcher(
        helper_words={'тв', 'tv', 'канал', 'channel', 'телеканал', 'телевидение', 'номер'},
        number_idf=0.5
    )
    item_texts = tv_selector.get_item_texts(
        {'name': gallery_text}, index=42, is_visible=True, current_screen='tv_gallery'
    )
    request_text = tv_selector.get_text_to_match(
        form=Form('form name', slots=[Slot(name='video_text', value=user_text, types=None)]),
        sample=None,
        req_info=create_request(123, device_state={'video': {'current_screen': 'tv_gallery'}})
    )
    scores = [matcher.get_score(item_text, request_text) for item_text in item_texts]
    assert min_score <= max(scores) <= max_score, 'match {} with [{}]:  max({}) not in [{}, {}]'.format(
        request_text, ', '.join(item_texts), scores, min_score, max_score
    )


@pytest.mark.parametrize(('raw_request', 'expected_response'), [
    ('да винчи', 'DA VINCI LEARNING'),
    ('DA VINCI LEARNING', 'да винчи'),
    ('трололо', 'трололо')
])
def test_get_synonyms(tv_selector_with_synonyms, raw_request, expected_response):
    response_text = tv_selector_with_synonyms.normalize_item_text(expected_response, transliterate=True)
    request_text = tv_selector_with_synonyms.normalize_item_text(raw_request, transliterate=True)
    assert response_text in tv_selector_with_synonyms.get_synonyms(request_text)


def test_cache():
    seen_texts = set()

    def extract(input_texts):
        text = input_texts[0]
        assert text not in seen_texts, 'The same text has been normalized twice'
        seen_texts.add(text)
        return [Sample(text)]

    selector = TVGalleryItemSelector()
    selector._extractor = extract
    texts_sequence = ['настоящий мистический', 'ненастоящий мистический', 'настоящий мистический']
    for name in texts_sequence:
        selector.get_item_texts({'name': name}, index=42, is_visible=True, current_screen='tv_gallery')
    assert seen_texts == set(texts_sequence)
