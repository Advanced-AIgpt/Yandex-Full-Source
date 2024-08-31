import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import server_action


@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.oauth(auth.YandexPlus)
class TestMusicInfiniteFeed:
    REQUIRED_INFINITE_FEED_FIELDS = [
        'inf_feed_auto_playlists',
        'inf_feed_tag_playlists',
        'inf_feed_popular_artists',
        'inf_feed_recommended_podcasts',
        'inf_feed_tag_playlists-for_kids',
    ]

    @pytest.mark.experiments(
        'mm_enable_combinators',
    )
    def test_infinite_feed(self, alice):
        payload = {
            'typed_semantic_frame': {
                'centaur_collect_main_screen': {
                }
            },
            'analytics': {
                'origin': 'SmartSpeaker',
                'purpose': 'centaur_collect_main_screen'
            }
        }

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.scenario_stages() == {'continue', 'run'}

        scenario_data = r.continue_response.ResponseBody.ScenarioData
        for required_block_type in self.REQUIRED_INFINITE_FEED_FIELDS:
            assert any(block.Type.startswith(required_block_type) for block in scenario_data.MusicInfiniteFeedData.MusicObjectsBlocks)
        for block in scenario_data.MusicInfiniteFeedData.MusicObjectsBlocks:
            assert len(block.MusicObjects) > 0

        return str(r)
