import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.scenarios.goods.proto.goods_pb2 import TGoodsState


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['goods']


LINK_SURFACES = [surface.navi, surface.maps, surface.launcher, surface.yabro_win]
SEARCH_VIEW_SURFACES = [surface.searchapp]
OTHER_SURFACES = [s for s in surface.actual_surfaces if s not in LINK_SURFACES and s not in SEARCH_VIEW_SURFACES]


@pytest.mark.scenario(name='Goods', handle='goods')
@pytest.mark.experiments('bg_fresh_granet=alice.goods.best_prices')
class _TestsGoodsBase:
    speech_variants_product_request = {
        'Обыскала весь интернет, вот лучшие цены.',
        'Провела исследование и нашла самые выгодные цены.',
        'Нашла товары по самым выгодным ценам.',
        'Обошла все магазины и нашла, где дешевле.',
    }
    speech_variants_main_page_request = {
        'Что вам помочь найти? Укажите товар, который нужен, и я найду, где дешевле.',
        'Чем вам помочь? Укажите товар, который нужен, и я найду, где дешевле.',
        'Ищете что-то конкретное? Укажите товар, который нужен, и я найду лучшие цены.',
        'Ищете что-то особенное? Укажите товар, который нужен, я найду, где дешевле.',
    }
    speech_variants_not_avail = {
        'Спросите меня об этом в приложении Яндекс и я найду где дешевле.',
        'Если спросите меня об этом в приложении Яндекс, я сделаю всё в лучшем виде.',
        'Пойдёмте в приложение Яндекс, я вам там все покажу.',
    }
    speech_variants_push = {
        'Нашла. Отправила вам ссылку в приложение Яндекс.',
        'Нашла, где дешевле, и отправила вам ссылку в приложение Яндекс.',
        'Пожалуйста, откройте приложение Яндекс, я уже прислала вам варианты.',
    }

    speech_variants_to_reask = {
        'Что вы хотели бы купить?',
        'Что для вас найти?',
        'Что вас интересует?',
        'Что вы ищете?',
    }

    suggest_texts = [
        'Кофемашина',
        'Наушники',
        'Умные часы',
        'Миксер',
        'Другие товары',
    ]

    action_id = 'goods_button_with_uri'

    def get_state(self, response):
        state = TGoodsState()
        response.ResponseBody.State.Unpack(state)
        return state

    def check_frame(self, response, expected):
        frame = response.run_response.ResponseBody.SemanticFrame.Name
        assert frame == expected

    def check_speech(self, response, expected):
        output_speech = response.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in expected

    def check_uri(self, response, expected):
        directives = response.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('OpenUriDirective')
        open_uri_directive = directives[0].OpenUriDirective
        assert open_uri_directive.Uri == expected
        frame_actions = response.run_response.ResponseBody.FrameActions
        assert len(frame_actions) == 1
        assert self.action_id in frame_actions.keys()
        assert frame_actions[self.action_id].Directives.List[0].OpenUriDirective.Uri == expected

    def check_relevant(self, response):
        assert not response.run_response.Features.IsIrrelevant

    def check_irrelevant(self, response):
        assert response.run_response.Features.IsIrrelevant

    def check_will_reask(self, response):
        state = self.get_state(response.run_response)
        assert state.IsReask
        assert response.run_response.ResponseBody.ExpectsRequest
        assert response.run_response.ResponseBody.Layout.ShouldListen
        suggests = response.run_response.ResponseBody.Layout.SuggestButtons
        assert(len(suggests) == 5)
        for i in range(len(suggests)):
            assert suggests[i].ActionButton.Title == self.suggest_texts[i]

    def check_will_not_reask(self, response):
        state = self.get_state(response.run_response)
        assert not state.IsReask
        assert not response.run_response.ResponseBody.ExpectsRequest
        assert not response.run_response.ResponseBody.Layout.ShouldListen

    def check_push(self, response, url):
        server_directives = response.run_response.ResponseBody.ServerDirectives
        assert len(server_directives) == 1
        push_directive = server_directives[0].PushMessageDirective
        assert push_directive.Link == url


@pytest.mark.parametrize('surface', LINK_SURFACES)
class TestsLinkSurfaces(_TestsGoodsBase):
    def test_show_link(self, alice):
        r = alice(voice('где дешевле айфон'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_product_request)
        self.check_uri(r, 'https://yandex.ru/products/search?query_source=alice&query_source=voice&text=%D0%B0%D0%B9%D1%84%D0%BE%D0%BD')
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.parametrize('surface', SEARCH_VIEW_SURFACES)
class TestsSearhViewSurfaces(_TestsGoodsBase):
    def test_show_link(self, alice):
        r = alice(voice('где дешевле айфон'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_product_request)
        self.check_uri(r, 'viewport://?noreask=1&query_source=alice&query_source=voice&text=%D0%B0%D0%B9%D1%84%D0%BE%D0%BD&viewport_id=products')
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.parametrize('surface', OTHER_SURFACES)
class TestsSearhOtherSurfacesNoLogin(_TestsGoodsBase):
    def test_not_avail(self, alice):
        r = alice(voice('где дешевле айфон'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_not_avail)
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', OTHER_SURFACES)
class TestsSearhOtherSurfacesLogin(_TestsGoodsBase):
    def test_push(self, alice):
        r = alice(voice('где дешевле айфон'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_push)
        self.check_relevant(r)
        self.check_will_not_reask(r)
        self.check_push(r, 'viewport://?noreask=1&query_source=alice&query_source=voice&text=%D0%B0%D0%B9%D1%84%D0%BE%D0%BD&viewport_id=products')
        return str(r)


@pytest.mark.parametrize('surface', LINK_SURFACES)
class TestsToReaskLinkSurfaces(_TestsGoodsBase):
    def test_reask(self, alice):
        r = alice(voice('где дешевле'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_to_reask)
        self.check_relevant(r)
        self.check_will_reask(r)
        r = alice(voice('айфон'))
        self.check_frame(r, 'alice.goods.best_prices_reask')
        self.check_speech(r, _TestsGoodsBase.speech_variants_product_request)
        self.check_uri(r, 'https://yandex.ru/products/search?query_source=alice&query_source=voice&text=%D0%B0%D0%B9%D1%84%D0%BE%D0%BD')
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.parametrize('surface', LINK_SURFACES)
class TestsMainPageLinkSurfaces(_TestsGoodsBase):
    def test_main_page(self, alice):
        r = alice(voice('где дешевле'))
        r = alice(voice('Другие товары'))
        self.check_speech(r, _TestsGoodsBase.speech_variants_main_page_request)
        self.check_uri(r, 'https://yandex.ru/products')
        return str(r)


@pytest.mark.parametrize('surface', SEARCH_VIEW_SURFACES)
class TestsToReaskSearchViewSurfaces(_TestsGoodsBase):
    def test_reask(self, alice):
        r = alice(voice('где дешевле'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_to_reask)
        self.check_relevant(r)
        self.check_will_reask(r)
        r = alice(voice('айфон'))
        self.check_frame(r, 'alice.goods.best_prices_reask')
        self.check_speech(r, _TestsGoodsBase.speech_variants_product_request)
        self.check_uri(r, 'viewport://?noreask=1&query_source=alice&query_source=voice&text=%D0%B0%D0%B9%D1%84%D0%BE%D0%BD&viewport_id=products')
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.parametrize('surface', SEARCH_VIEW_SURFACES)
class TestsMainPageSearchViewSurfaces(_TestsGoodsBase):
    def test_main_page(self, alice):
        r = alice(voice('где дешевле'))
        r = alice(voice('Другие товары'))
        self.check_speech(r, _TestsGoodsBase.speech_variants_main_page_request)
        self.check_uri(r, 'viewport://?noreask=1&text=%20&viewport_id=products')
        return str(r)


@pytest.mark.parametrize('surface', OTHER_SURFACES)
class TestsToReaskOtherSurfacesNoLogin(_TestsGoodsBase):
    def test_reask(self, alice):
        r = alice(voice('где дешевле'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_to_reask)
        self.check_relevant(r)
        self.check_will_reask(r)
        r = alice(voice('айфон'))
        self.check_frame(r, 'alice.goods.best_prices_reask')
        self.check_speech(r, _TestsGoodsBase.speech_variants_not_avail)
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', OTHER_SURFACES)
class TestsToReaskOtherSurfacesLogin(_TestsGoodsBase):
    def test_reask(self, alice):
        r = alice(voice('где дешевле'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_speech(r, _TestsGoodsBase.speech_variants_to_reask)
        self.check_relevant(r)
        self.check_will_reask(r)
        r = alice(voice('айфон'))
        self.check_frame(r, 'alice.goods.best_prices_reask')
        self.check_speech(r, _TestsGoodsBase.speech_variants_push)
        self.check_push(r, 'viewport://?noreask=1&query_source=alice&query_source=voice&text=%D0%B0%D0%B9%D1%84%D0%BE%D0%BD&viewport_id=products')
        self.check_relevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsNotProduct(_TestsGoodsBase):
    def test_not_product(self, alice):
        r = alice(voice('где дешевле билеты в театр'))
        self.check_frame(r, 'alice.goods.best_prices')
        self.check_irrelevant(r)
        self.check_will_not_reask(r)
        return str(r)


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestsIrrelevant(_TestsGoodsBase):
    def test_irrelevant_frame(self, alice):
        r = alice(voice('айфон'))
        self.check_frame(r, 'alice.goods.best_prices_reask')
        self.check_irrelevant(r)
        self.check_will_not_reask(r)
        return str(r)
