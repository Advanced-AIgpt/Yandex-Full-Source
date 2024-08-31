import json
import logging
import re

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice


logger = logging.getLogger(__name__)

EXPERIMENTS = [
    'hw_music_thin_client',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
class _TestsBitrateBase:
    pass


@pytest.mark.parametrize('surface', [surface.loudspeaker])
class _TestsBitrateSimpleTestBase(_TestsBitrateBase):
    def test(self, alice):
        r = alice(voice('включи рамштайн'))
        layout = r.continue_response.ResponseBody.Layout

        assert layout.Directives
        assert layout.Directives[0].HasField('AudioPlayDirective')

        # TODO(sparkle): HOLLYWOOD-801 check /download-info request


@pytest.mark.supported_features('audio_bitrate320')  # should use 320 Kbps stream
class TestsBitrate320(_TestsBitrateSimpleTestBase):
    pass


@pytest.mark.supported_features('audio_bitrate192', 'audio_bitrate320')  # should use 192 Kbps stream, because it is a SMALL speaker
class TestsBitrate192(_TestsBitrateSimpleTestBase):
    pass


@pytest.mark.experiments('hw_music_use_download_info_format_flags')
@pytest.mark.supported_features('audio_bitrate192', 'audio_bitrate320')
class TestsBitrateFormatFlags(_TestsBitrateBase):

    HTTP_NODE_NAME = 'MUSIC_SCENARIO_THIN_DOWNLOAD_INFO_MP3_GET_ALICE_PROXY'

    @pytest.mark.parametrize('flag, bitrate, surface', [
        pytest.param('formatFlags=hq', 320, surface.station, id='hq'),  # should use 320 Kbps stream
        pytest.param('formatFlags=lq', 192, surface.loudspeaker, id='lq'),  # should use 192 Kbps stream
        pytest.param('formatFlags=hq', 320, surface.station_midi, id='hq_midi')  # should use 320 Kbps stream
    ])
    def test(self, alice, flag, bitrate):
        '''
        Запрос в ручку "/download-info" пойдет с параметром "formatFlags=lq"
        или "formatFlags=hq", и вернет только один битрейт
        '''
        r = alice(voice('включи рамштайн'))
        layout = r.continue_response.ResponseBody.Layout

        assert layout.Directives
        assert layout.Directives[0].HasField('AudioPlayDirective')

        # example of path: "/tracks/51422266/download-info?isAliceRequester=true&__uid=1083955728&formatFlags=lq"
        mp3_req = r.sources_dump.get_http_request(self.HTTP_NODE_NAME)
        assert re.match(rf'\/internal-api\/tracks\/\d+\/download-info\?isAliceRequester=true&__uid=\d+&{flag}', mp3_req.path)

        # there should be only one result in response with target bitrate
        mp3_resp = r.sources_dump.get_http_response(self.HTTP_NODE_NAME)
        content = json.loads(mp3_resp.content)
        result = content['result']
        assert len(result) == 1
        assert result[0]['bitrateInKbps'] == bitrate
