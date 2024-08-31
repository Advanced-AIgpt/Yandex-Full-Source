import logging
import pytest

from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import server_action, voice
from alice.hollywood.library.python.testing.it2.stubber import create_localhost_bass_stubber_fixture


logger = logging.getLogger(__name__)
bass_stubber = create_localhost_bass_stubber_fixture()


EXPERIMENTS = [
    'hw_music_thin_client',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.station])
class _TestsBase:
    pass


class TestsRemoveLikeOrDislike(_TestsBase):

    # 1083955728 - user id
    # 15740368 - album id "Музыка для мужчин"
    # 83526208 - track id "Я на тебе никогда не женюсь"
    # https://music.yandex.ru/album/15740368/track/83526208
    @pytest.mark.parametrize('frame_name, http_proxy_node, http_path', [
        pytest.param(
            'player_remove_like_semantic_frame',
            'MUSIC_COMMIT_REMOVE_LIKE_PROXY',
            '/internal-api/users/1083955728/likes/tracks/83526208:15740368/remove?__uid=1083955728',
            id='remove_like',
        ),
        pytest.param(
            'player_remove_dislike_semantic_frame',
            'MUSIC_COMMIT_REMOVE_DISLIKE_PROXY',
            '/internal-api/users/1083955728/dislikes/tracks/83526208:15740368/remove?__uid=1083955728',
            id='remove_dislike',
        ),
    ])
    def test_current_track(self, alice, frame_name, http_proxy_node, http_path):
        r = alice(voice('включи укупник я на тебе никогда не женюсь'))

        payload = {
            'typed_semantic_frame': {
                frame_name: {
                },
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'change_like_status',
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert r.scenario_stages() == {'run', 'commit'}

        http_req = r.sources_dump.get_http_request(http_proxy_node)
        assert http_req
        assert http_req.path == http_path

        http_resp = r.sources_dump.get_http_response(http_proxy_node)
        assert http_resp
        assert http_resp.status_code == 200

    # 1083955728 - user id
    # 151737 - album id "ОСТ "Бригада""
    # 1695407 - track id "Бригада. Пролог"
    # https://music.yandex.ru/album/151737/track/1695407
    @pytest.mark.parametrize('frame_name, http_proxy_node, http_path', [
        pytest.param(
            'player_remove_like_semantic_frame',
            'MUSIC_COMMIT_REMOVE_LIKE_PROXY',
            '/internal-api/users/1083955728/likes/tracks/1695407:151737/remove?__uid=1083955728',
            id='remove_like',
        ),
        pytest.param(
            'player_remove_dislike_semantic_frame',
            'MUSIC_COMMIT_REMOVE_DISLIKE_PROXY',
            '/internal-api/users/1083955728/dislikes/tracks/1695407:151737/remove?__uid=1083955728',
            id='remove_dislike',
        ),
    ])
    def test_content_id_track(self, alice, frame_name, http_proxy_node, http_path):
        payload = {
            'typed_semantic_frame': {
                frame_name: {
                    'content_id': {
                        'content_id_value': {
                            'type': 'Track',
                            'id': '1695407:151737',
                        }
                    }
                },
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'change_like_status',
            }
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        http_req = r.sources_dump.get_http_request(http_proxy_node)
        assert http_req
        assert http_req.path == http_path

        http_resp = r.sources_dump.get_http_response(http_proxy_node)
        assert http_resp
        assert http_resp.status_code == 200
