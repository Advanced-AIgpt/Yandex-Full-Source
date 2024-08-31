import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint, create_stubber_fixture, HttpResponseStub


logger = logging.getLogger(__name__)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['subscriptions_manager']


billing_stubber = create_stubber_fixture(
    'paskills-common-testing.alice.yandex.net',
    80,
    [
        StubberEndpoint('/billing/requestPlus', ['POST']),
    ],
    scheme='http',
    stubs_subdir='billing',
)

passport_info_user_stubber = create_stubber_fixture(
    'api.mt.mediabilling.yandex.net',
    80,
    [
        StubberEndpoint('/passport-info/user', ['GET']),
    ],
    scheme='http',
    stubs_subdir='passport_info_user',
)


@pytest.fixture(scope='function')
def srcrwr_params(billing_stubber, passport_info_user_stubber):
    return {
        'PASKILLS_COMMON_PROXY': f'localhost:{billing_stubber.port}',
        'ALICE__MEDIABILLING_PROXY': f'localhost:{passport_info_user_stubber.port}',
    }


@pytest.mark.scenario(name='SubscriptionsManager', handle='subscriptions_manager')
@pytest.mark.experiments('bg_fresh_granet')
class _TestSubscriptionsManager:
    def _test_base(self, alice, command, intent, text=None, speech=None, uri=None, uri_title='Открыть', uri_text=None):
        r = alice(voice(command))
        assert r.scenario_stages() == {'run', 'continue'}
        response = r.continue_response.ResponseBody
        layout = response.Layout
        has_suggest = uri is not None and uri_text is None
        if text is not None:
            card = layout.Cards[0]
            if has_suggest:
                assert card.TextWithButtons.Text == text
                assert card.TextWithButtons.Buttons[0].Title == uri_title
            else:
                assert card.Text == text
        if speech is not None:
            assert layout.OutputSpeech == speech
        if uri is not None:
            if has_suggest:
                directives = layout.Directives
                assert len(directives) == 1
                assert directives[0].WhichOneof('Directive') == 'OpenUriDirective'
                dir = directives[0].OpenUriDirective
                assert dir.Name == 'open_uri'
                assert dir.Uri == uri
            else:
                directives = response.ServerDirectives
                assert len(directives) == 1
                assert directives[0].WhichOneof('Directive') == 'SendPushDirective'
                dir = directives[0].SendPushDirective
                assert dir.PushId == 'subscriptions_manager'
                assert dir.PushTag == 'subscriptions_manager'
                assert not dir.RemoveExistingCards
                assert dir.PushMessage.ThrottlePolicy == 'unlimited_policy'
                settings = dir.Settings
                assert settings.Title == uri_title
                assert settings.Text == uri_text
                assert settings.Link == uri
                assert settings.TtlSeconds == 180
        analytics = r.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == intent
        assert analytics.ProductScenarioName == 'subscriptions_manager'


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station])
class TestSubscriptionsManagerHowTo(_TestSubscriptionsManager):
    def _test(self, alice, text=None, speech=None, uri=None, uri_title='Открыть', uri_text=None):
        self._test_base(alice, 'как оформить подписку яндекс плюс', 'alice.subscriptions.how_to_subscribe', text, speech, uri, uri_title, uri_text)

    @pytest.mark.supported_features('open_link', 'open_link_yellowskin')
    @pytest.mark.freeze_stubs(billing_stubber={
        '/billing/requestPlus': [HttpResponseStub(200, content='''
            {
                "result": "PROMO_AVAILABLE",
                "activate_promo_uri": "http://not.a.real.url"
            }
        ''')],
    })
    def test_open_link__has_promo(self, alice):
        self._test(alice, text='Настройка подписок Алисы', speech='Открываю', uri='yellowskin://?url=https%3A//yandex.ru/quasar/purchases')

    @pytest.mark.supported_features('open_link')
    def test_open_link__no_promo(self, alice):
        self._test(alice, text='Яндекс Плюс. Настройка подписки', speech='Открываю', uri='https://plus.yandex.ru')

    @pytest.mark.freeze_stubs(billing_stubber={
        '/billing/requestPlus': [HttpResponseStub(200, content='''
            {
                "result": "PROMO_AVAILABLE",
                "activate_promo_uri": "http://not.a.real.url"
            }
        ''')],
    })
    def test_response__has_promo(self, alice):
        self._test(alice, speech='Чтобы активировать промо-подписку, зайдите в мобильное приложение Яндекса '
                                 'в вашем телефоне и скажите мне: "Алиса, активируй Яндекс Плюс".',
                   uri='yellowskin://?url=https%3A//yandex.ru/quasar/purchases', uri_title='Яндекс.Плюс',
                   uri_text='Нажмите для активации Яндекс.Плюс')

    def test_response__no_promo(self, alice):
        self._test(alice, speech='Пожалуйста, откройте приложение Яндекса в вашем телефоне и зайдите в раздел '
                                 '"Плюс". Выберите подходящий тариф и оплатите его.',
                   uri='https://plus.yandex.ru', uri_title='Яндекс.Плюс',
                   uri_text='Нажмите для активации Яндекс.Плюс')


@pytest.mark.parametrize('surface', [surface.station])
class TestSubscriptionsManagerStatus(_TestSubscriptionsManager):
    def _test(self, alice, text=None, speech=None, uri=None, uri_title='Открыть', uri_text=None):
        self._test_base(alice, 'информация о подписке яндекс плюс', 'alice.subscriptions.status', text, speech, uri, uri_title, uri_text)

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.supported_features('open_link')
    def test_open_link(self, alice):
        self._test(alice, text='Яндекс Плюс. Статус подписки', speech='Открываю', uri="https://plus.yandex.ru/my")

    @pytest.mark.freeze_stubs(passport_info_user_stubber={
        '/passport-info/user': [HttpResponseStub(200, 'freezed_stubs/get_passport-info-user2.json')],
    })
    @pytest.mark.oauth(auth.YandexPlus)
    def test_response__has_plus(self, alice):
        self._test(alice, speech='До 8 февраля у вас активна подписка Плюс Мульти. Если хотите '
                                 'узнать больше, откройте приложение Яндекса. Прислала ссылку вам в телефон.',
                   uri='https://plus.yandex.ru/my', uri_title='Яндекс.Плюс',
                   uri_text='Список ваших подписок')

    @pytest.mark.freeze_stubs(passport_info_user_stubber={
        '/passport-info/user': [HttpResponseStub(200, 'freezed_stubs/get_passport-info-user.json')],
    })
    @pytest.mark.oauth(auth.YandexPlus)
    def test_response__has_plus__multi_with_subscription_word(self, alice):
        self._test(alice, speech='До 8 февраля у вас активна подписка Плюс Мульти и до 20 июня 2021 года '
                                 'у вас активна Станция с подпиской. Если хотите узнать больше, откройте '
                                 'приложение Яндекса. Прислала ссылку вам в телефон.',
                   uri='https://plus.yandex.ru/my', uri_title='Яндекс.Плюс',
                   uri_text='Список ваших подписок')

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.freeze_stubs(billing_stubber={
        '/billing/requestPlus': [HttpResponseStub(200, content='''
            {
                "result": "PROMO_AVAILABLE",
                "activate_promo_uri": "http://not.a.real.url"
            }
        ''')],
    })
    def test_response__no_plus__has_promo(self, alice):
        self._test(alice, speech='На текущий момент у вас нет активных подписок, но вам доступен бесплатный '
                                 'промопериод, приложенный к устройству. Откройте приложение Яндекса для его '
                                 'активации. Прислала ссылку вам в телефон.',
                   uri='yellowskin://?url=https%3A//yandex.ru/quasar/purchases', uri_title='Яндекс.Плюс',
                   uri_text='Нажмите для активации Яндекс.Плюс')

    @pytest.mark.oauth(auth.Yandex)
    def test_response__no_plus__no_promo(self, alice):
        self._test(alice, speech='На текущий момент у вас нет активных подписок. Если хотите оформить, откройте '
                                 'приложение Яндекса. Прислала ссылку вам в телефон.',
                   uri='https://plus.yandex.ru/my', uri_title='Яндекс.Плюс',
                   uri_text='Нажмите для активации Яндекс.Плюс')


@pytest.mark.oauth(auth.Yandex)
class TestSubscriptionsManagerWhatWithout(_TestSubscriptionsManager):
    def _test(self, alice, text=None, speech=None):
        self._test_base(alice, 'что ты можешь без подписки', 'alice.subscriptions.what_can_you_do_without_subscription', text, speech)

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(is_tv_plugged_in=True)
    def test_response__has_screen(self, alice):
        self._test(alice, speech='Без подписки Яндекс Плюс вы не сможете слушать музыку, сказки, а также '
                                 'смотреть фильмы и сериалы из КиноПоиска, которые входят в подписку.')

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_response__no_screen(self, alice):
        self._test(alice, speech='Без подписки Яндекс Плюс вы не сможете слушать музыку, сказки и звуки природы.')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_response__other(self, alice):
        self._test(alice, speech='Без подписки Яндекс Плюс контент Яндекс Музыки будет ограничен, но вы сможете '
                                 'включить радио, узнать свежие новости или прогноз погоды.')
