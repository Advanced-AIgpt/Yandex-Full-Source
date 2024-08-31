import json

import alice.tests.library.auth as auth
import alice.tests.library.surface as surface
import alice.tests.library.ydb as ydb
import pytest


SURFACE_MAP = {
    'quasar': surface.station,
    'small_smart_speakers': surface.loudspeaker,
    'search_app_prod': surface.searchapp,
    'browser_prod': surface.yabro_win,
    'navigator': surface.navi,
    'other': surface.aliceapp,
    'yabro_prod': surface.yabro_win,
    'auto': surface.automotive,
    'auto_old': surface.old_automotive,
    'yandexmaps_prod': surface.navi,
}


def _make_parametrize():
    params = ydb.download_parametrize()
    return [pytest.param(
        SURFACE_MAP[p.app_preset](**json.loads(p.device_state)),
        p.text,
        p,
        marks=[
            pytest.mark.experiments(p.experiments),
            pytest.mark.region(timezone=p.timezone, **json.loads(p.location)),
        ],
        id=f'{p.app_preset}-{p.text}_{p.real_reqid}',
    ) for p in params]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface, command, expected_response', _make_parametrize())
class Test(object):
    def test(self, alice, command, expected_response):
        response = alice(command)
        assert response.product_scenario == expected_response.product_scenario
        assert response.scenario == expected_response.scenario

        directives = [d.name for d in response.directives]
        expected_directives = [d['name'] for d in json.loads(expected_response.directives)]
        assert directives == expected_directives

        card_types = [c.type for c in response.cards]
        expected_card_types = [c['type'] for c in json.loads(expected_response.cards)]
        assert card_types == expected_card_types
