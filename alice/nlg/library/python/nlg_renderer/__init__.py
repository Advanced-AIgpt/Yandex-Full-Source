from alice.nlg.library.python.nlg_renderer.bindings import (  # noqa
    create_nlg_renderer_from_nlg_library_path,
    AliceRng, RenderContextData
)


def create_alice_rng(seed=None):
    return AliceRng(seed)


def create_render_context_data(context=None, form=None, req_info=None):
    return RenderContextData(context or {}, form or {}, req_info or {})


class Language(object):
    AR = 'ar'
    EN = 'en'
    RU = 'ru'
