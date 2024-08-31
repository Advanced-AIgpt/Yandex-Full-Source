import pytest
from alice.hollywood.library.python.testing.it2.input import voice, server_action

from conftest import load_json_device_state, YaModuleTestingPreset, VideoScenarioTestingPreset


@pytest.mark.device_state(load_json_device_state('fixtures/searchgallery_first-ott.json'))
class TestSearchResultsOTTGallery(YaModuleTestingPreset):
    # Открытие описания по номеру
    def test_open_content_details_by_number(self, alice):
        r = alice(voice("номер два"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'
        assert directives[0].TvOpenDetailsScreenDirective.VhUuid == "4c8ac4db38abb1d285a55e016da54951"

        return str(r)

    # Открытие описания по тексту
    def test_open_content_details_by_text(self, alice):
        r = alice(voice("Джон Уик три"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'
        assert directives[0].TvOpenDetailsScreenDirective.VhUuid == "4b873f50b3df8fe78a2d7520e681441a"

        return str(r)

    # Проигрывание (есть лицензии) по номеру
    def test_play_by_number(self, alice):
        r = alice(voice("Включи номер два"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Item.ProviderItemId == "4c8ac4db38abb1d285a55e016da54951"

        return str(r)

    # Проигрывание (есть лицензии) по тексту
    def test_play_by_text(self, alice):
        r = alice(voice("Включи Джон Уик три"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Item.ProviderItemId == "4b873f50b3df8fe78a2d7520e681441a"

        return str(r)

    # Открытие описания вместо проигрывания (нет лицензий) по тексту
    def test_open_content_details_no_licenses_by_text(self, alice):
        r = alice(voice("Включи Джон Уик"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'
        assert directives[0].TvOpenDetailsScreenDirective.VhUuid == "496a6a063cc043a194528e8dd80cfad6"

        return str(r)


@pytest.mark.device_state(load_json_device_state('fixtures/private_practice_in_external_gallery.json'))
class TestSearchResultsExternalGalleryWithTvShow(YaModuleTestingPreset):
    def test_play_by_number(self, alice):
        r = alice(voice("открой номер 1"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'
        assert directives[0].TvOpenDetailsScreenDirective.VhUuid == "4384a6554daea6de85baec4f97e5e175"

        return str(r)

    def test_play_by_text(self, alice):
        r = alice(voice("открой практика"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'
        assert directives[0].TvOpenDetailsScreenDirective.VhUuid == "4384a6554daea6de85baec4f97e5e175"

        return str(r)


@pytest.mark.device_state(load_json_device_state('fixtures/searchgallery_second-externalvideo.json'))
class TestSearchResultsExternalGallery(YaModuleTestingPreset):
    def test_play_by_number(self, alice):
        r = alice(voice("номер один"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Uri == "https://ok.ru/videoembed/2121975859959?autoplay=1&ya=1"
        assert directives[0].VideoPlayDirective.Item.ProviderName == "yavideo"
        assert directives[0].VideoPlayDirective.Item.ProviderItemId == "http://ok.ru/video/2121975859959"

        return str(r)

    def test_play_by_text(self, alice):
        r = alice(voice("Джон Уик русский трейлер"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert (
            directives[0].VideoPlayDirective.Uri
            == "https://vk.com/video_ext.php?autoplay=1&hash=4ecdff22346de1a5&id=456241108&loop=0&oid=-116922324"
        )
        assert directives[0].VideoPlayDirective.Item.ProviderName == "yavideo"
        assert directives[0].VideoPlayDirective.Item.ProviderItemId == "http://vk.com/video-116922324_456241108"

        return str(r)


@pytest.mark.device_state(load_json_device_state('fixtures/searchgallery_second-person.json'))
class TestSearchResultsPersonGallery(YaModuleTestingPreset):
    def test_open_person_by_number(self, alice):
        r = alice(voice("номер два"))
        return str(r)

    def test_open_person_by_text(self, alice):
        r = alice(voice("Эмма Стоун"))
        return str(r)


@pytest.mark.device_state(load_json_device_state('fixtures/searchgallery_third-collection.json'))
class TestSearchResultsCollectionGallery(YaModuleTestingPreset):
    def test_open_collection_by_number(self, alice):
        r = alice(voice("номер два"))
        return str(r)

    def test_open_collection_by_text(self, alice):
        r = alice(voice("Фильмы про волшебство"))
        return str(r)


@pytest.mark.device_state(load_json_device_state('fixtures/expanded_collection.json'))
class TestExpandedCollection(YaModuleTestingPreset):
    # Открытие описания по номеру
    def test_open_content_details_by_number(self, alice):
        r = alice(voice("номер восемь"))
        return str(r)

    # Открытие описания по номеру (не видно на экране)
    def test_open_content_details_not_visible_by_number(self, alice):
        r = alice(voice("номер два"))
        return str(r)

    # Проигрывание (есть лицензии) по номеру
    def test_play_by_number(self, alice):
        r = alice(voice("Включи номер восемь"))
        return str(r)

    # Проигрывание (есть лицензии) (не видно на экране) по номеру
    def test_play_not_visible_by_number(self, alice):
        r = alice(voice("Включи номер два"))
        return str(r)

    # Открытие описания вместо проигрывания (нет лицензий) по тексту
    def test_open_content_details_no_licenses_by_text(self, alice):
        r = alice(voice("Включи алоха"))
        return str(r)


@pytest.mark.experiments('gallery_video_select', 'video_use_pure_hw')
class TestItemSelection(VideoScenarioTestingPreset):
    def _get_response_vh(self, alice, provider_item_id, action):
        payload = {
            'typed_semantic_frame': {
                'gallery_video_select_semantic_frame': {
                    'action': {
                        'string_value': action
                    },
                    'provider_item_id': {
                        'string_value': provider_item_id
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Video",
                "purpose": "select_video_from_gallery"
            }
        }
        return alice(server_action(name='@@mm_semantic_frame', payload=payload))

    # открыть описание фильма с Кинопоиска
    def test_open_vh(self, alice):
        r = self._get_response_vh(alice, '48ec62883cb1bfc8a65154fcd3749b72', "open")
        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'
        assert directives[0].TvOpenDetailsScreenDirective.VhUuid == "48ec62883cb1bfc8a65154fcd3749b72"
        return str(r)

    '''
    def test_play_vh(self, alice):
        r = self._get_response_vh(alice, '48ec62883cb1bfc8a65154fcd3749b72', 'play')

        assert r is not None
        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives

        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Item.ProviderItemId == '48ec62883cb1bfc8a65154fcd3749b72'
    '''

    def _get_response(self, alice, embed_uri, action):
        payload = {
            'typed_semantic_frame': {
                'gallery_video_select_semantic_frame': {
                    'action': {
                        'string_value': action
                    },
                    'embed_uri': {
                        'string_value': embed_uri
                    }
                }
            },
            "analytics": {
                "origin": "Scenario",
                "product_scenario": "Video",
                "purpose": "select_video_from_gallery"
            }
        }
        return alice(server_action(name='@@mm_semantic_frame', payload=payload))

    # включить фильм с сайта
    def test_play(self, alice):
        r = self._get_response(alice, 'https://frontend.vh.yandex.ru/player/13991786694980472246?autoplay=1&amp;service=ya-video&amp;from=yavideo', "open")
        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Uri == "https://frontend.vh.yandex.ru/player/13991786694980472246?autoplay=1&amp;service=ya-video&amp;from=yavideo"
        return str(r)
