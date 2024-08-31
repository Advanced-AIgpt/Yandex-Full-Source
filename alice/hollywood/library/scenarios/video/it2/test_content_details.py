import pytest
from alice.hollywood.library.python.testing.it2.input import voice

from conftest import YaModuleTestingPreset


@pytest.mark.device_state(
    {
        'video': {
            "current_screen": "content_details",
            'tv_interface_state': {
                'content_details_screen': {
                    'current_item': {
                        "provider_name": "kinopoisk",
                        "search_query": "однажды в османской империи: смута сериал",
                    }
                },
            },
        }
    }
)
class TestPirateCard(YaModuleTestingPreset):
    def test_pirate(self, alice):
        r = alice(voice("смотреть"))
        return str(r)


@pytest.mark.device_state(
    {
        "video": {
            "current_screen": "content_details",
            "tv_interface_state": {
                "content_details_screen": {
                    "current_item": {
                        "age_limit": 0,
                        "item_type": "movie",
                        "provider_item_id": "451faf4d11fb7fc5b2e1dc964b6e49fb",
                        "provider_name": "kinopoisk",
                    }
                }
            },
        }
    }
)
class TestVideoCard(YaModuleTestingPreset):
    def test_ott_with_flag(self, alice):
        r = alice(voice("смотреть"))
        layout = r.run_response.ResponseBody.Layout
        directives = layout.Directives
        assert len(directives) == 1
        return str(r)


@pytest.mark.device_state(
    {
        "video": {
            "current_screen": "content_details",
            "tv_interface_state": {
                "content_details_screen": {
                    "current_item": {
                        "age_limit": 12,
                        "item_type": "movie",
                        "provider_item_id": "4df0fe0d1c7bc66e88bb6848a1e926fd",
                        "provider_name": "kinopoisk",
                    }
                }
            },
        }
    }
)
class TestPaidVideoCard(YaModuleTestingPreset):
    def test_paid_ott(self, alice):
        r = alice(voice("смотреть"))
        return str(r)
