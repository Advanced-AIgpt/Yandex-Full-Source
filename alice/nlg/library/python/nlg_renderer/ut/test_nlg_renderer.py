from alice.nlg.library.python.nlg_renderer import (
    create_nlg_renderer_from_nlg_library_path,
    create_alice_rng, create_render_context_data,
    Language
)


NLG_LIBRARY_PATH = 'alice/nlg/library/python/nlg_renderer/ut/nlg'


def test_render_phrase():
    nlg_renderer = create_nlg_renderer_from_nlg_library_path(NLG_LIBRARY_PATH, create_alice_rng(1))
    render_phrase_result = nlg_renderer.render_phrase(
        'test_intent', 'happy_phrase', Language.RU,
        create_alice_rng(2), create_render_context_data({'x': 'foo'}))
    assert render_phrase_result.Text == 'Happy foo text'
    assert render_phrase_result.Voice == 'Happy foo voice'
