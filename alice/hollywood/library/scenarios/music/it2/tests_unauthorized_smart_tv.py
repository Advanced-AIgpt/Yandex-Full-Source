import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice
from hamcrest import assert_that, has_entries, contains, has_key


logger = logging.getLogger(__name__)

SEMANTIC_FRAME_PAYLOAD = {
    'typed_semantic_frame': {
        'music_play_semantic_frame': {
            'object_id': {
                'string_value': '103372440:2048'
            },
            'object_type': {
                'enum_value': 'Playlist'
            }
        }
    },
    'analytics': {
        'origin': 'SmartSpeaker',
        'purpose': 'play_music'
    }
}


@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.supported_features('music_quasar_client', 'unauthorized_music_directives', 'audio_client')
@pytest.mark.experiments('music_check_plus_promo')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class _TestsUnauthSmartTvBase:

    def _check_directive(self, response, directive_name, check_continue_response=True):
        obj = response.continue_response_pyobj if check_continue_response else response.run_response_pyobj
        root_key = 'response' if check_continue_response else 'response_body'
        assert_that(obj, has_entries({
            root_key: has_entries({
                'layout': has_entries({
                    'directives': contains(has_key(directive_name)),
                }),
            }),
        }))

    @pytest.mark.parametrize('command', [
        pytest.param('алиса включи', id='play_music'),
        pytest.param('включи queen', id='play_artist'),
        pytest.param('продолжи', id='continue'),
    ])
    def test_not_logged_in(self, alice, command):
        '''
        Любой запрос от незалогиненного пользователя должен иметь
        в ответе директиву "show_login"
        '''
        r = alice(voice(command))
        self._check_directive(r, 'show_login')

        expected = [
            'Кажется, вы не авторизованы. Войдите в свой аккаунт на Яндексе, чтобы слушать музыку.',
            'Чтобы слушать музыку, нужно авторизоваться. Пожалуйста, войдите в свой аккаунт на Яндексе.',
        ]
        assert r.continue_response.ResponseBody.Layout.OutputSpeech in expected

    @pytest.mark.oauth(auth.Yandex)
    def test_no_plus_no_promo(self, alice):
        '''
        Любой запрос от пользователя без Плюса должен иметь
        в ответе директиву "show_plus_purchase"
        '''
        r = alice(voice('включи queen'))
        self._check_directive(r, 'show_plus_purchase')

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.experiments('test_music_skip_plus_promo_check')  # emulates presense of promo
    def test_no_plus_but_promo(self, alice):
        '''
        Любой запрос от пользователя без Плюса, но с промокодом должен иметь
        в ответе директиву "show_plus_promo"

        В этом тесте промокод эмулируется через флаг
        '''
        r = alice(voice('включи queen'))
        self._check_directive(r, 'show_plus_promo')

    @pytest.mark.oauth(auth.YandexPlus)
    def test_plus(self, alice):
        '''
        Если Плюс есть, то эти директивы не должны создаваться
        '''
        r = alice(voice('включи queen'))
        continue_obj = r.continue_response_pyobj
        directives = continue_obj.get('response', {}).get('layout', {}).get('directives', {})
        assert not any(name in directives for name in [
            'show_login',
            'show_plus_purchase',
            'show_plus_promo',
        ])

    def test_via_frame_not_logged_in(self, alice, request):
        if isinstance(self, TestsUnauthSmartTvThinClient):
            request.applymarker(pytest.mark.xfail(reason='Test is dead after recanonization'))
        r = alice(server_action(name='@@mm_semantic_frame', payload=SEMANTIC_FRAME_PAYLOAD))
        self._check_directive(r, 'show_login', check_continue_response=False)

    @pytest.mark.oauth(auth.Yandex)
    def test_via_frame_no_plus_no_promo(self, alice, request):
        if isinstance(self, TestsUnauthSmartTvThinClient):
            request.applymarker(pytest.mark.xfail(reason='Test is dead after recanonization'))
        r = alice(server_action(name='@@mm_semantic_frame', payload=SEMANTIC_FRAME_PAYLOAD))
        self._check_directive(r, 'show_plus_purchase', check_continue_response=False)

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.experiments('test_music_skip_plus_promo_check')  # emulates presense of promo
    def test_via_frame_no_plus_but_promo(self, alice, request):
        if isinstance(self, TestsUnauthSmartTvThinClient):
            request.applymarker(pytest.mark.xfail(reason='Test is dead after recanonization'))
        r = alice(server_action(name='@@mm_semantic_frame', payload=SEMANTIC_FRAME_PAYLOAD))
        self._check_directive(r, 'show_plus_promo', check_continue_response=False)

    @pytest.mark.oauth(auth.YandexPlus)
    def test_via_frame_plus(self, alice):
        r = alice(server_action(name='@@mm_semantic_frame', payload=SEMANTIC_FRAME_PAYLOAD))
        run_obj = r.run_response_pyobj
        directives = run_obj.get('response', {}).get('layout', {}).get('directives', {})
        assert not any(name in directives for name in [
            'show_login',
            'show_plus_purchase',
            'show_plus_promo',
        ])


@pytest.mark.experiments('hw_music_thin_client', 'hw_music_thin_client_playlist')
class TestsUnauthSmartTvThinClient(_TestsUnauthSmartTvBase):
    pass


class TestsUnauthSmartTvThickClient(_TestsUnauthSmartTvBase):
    pass
