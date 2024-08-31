import argparse

import voicetech.common.lib.utils as utils

import yt.wrapper as yt

logger = utils.initialize_logging(__name__)

FORMAT = '%Y%m%dT%H%M%S'


@yt.yt_dataclass
class RequestRow:
    request_id: str


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('requests_table', help='//path/to/table/with/requests')
    parser.add_argument('wonder_table', help='//path/to/yt/table/wonder')
    parser.add_argument('result_table', help='//path/to/yt/result')
    return parser.parse_args()


def _get(a, *ids):
    for id in ids:
        if a is None:
            return None
        if isinstance(id, int):
            if id < len(a):
                a = a[id]
            else:
                return None
        else:
            if id in a:
                a = a[id]
            else:
                return None
    return a


class Mapper:
    def __init__(self, interesting_message_ids):
        self._interesting_message_ids = interesting_message_ids

    def __call__(self, row):
        mds_key = _get(row, 'asr', 'data', 'mds_key')
        if mds_key is None:
            return

        asr_words = _get(row, 'asr', 'data', 'recognition', 'words')
        if asr_words is None:
            asr_text = None
        else:
            asr_text = ' '.join(asr_words)

        spotter_mds_url = _get(row, 'spotter', 'mds_url')
        if spotter_mds_url is None:
            spotter_mds_key = None
        else:
            tokens = spotter_mds_url.split('/')
            spotter_mds_key = '/'.join(tokens[-2:])
            spotter_mds_url = 'https://speechbase.voicetech.yandex-team.ru/getaudio/{}?norm=1'.format(spotter_mds_key)

        device_state = _get(row, 'speechkit_request', 'request', 'device_state')
        cards = _get(row, 'speechkit_response', 'response', 'cards')
        if cards is not None:
            cards = [x['text'] for x in cards]

        directives = _get(row, 'speechkit_response', 'response', 'directives')
        if directives is not None:
            directives = ['{} {}'.format(x['name'], x['sub_name']) for x in directives]

        result = {
            'mds_key': mds_key,
            'asr_mds_url': 'https://speechbase.voicetech.yandex-team.ru/getaudio/{}?storage-type=s3&s3-bucket=voicelogs&norm=1'.format(
                mds_key
            ),
            'spotter_mds_key': spotter_mds_key,
            'spotter_mds_url': spotter_mds_url,
            'activation_type': _get(row, 'asr', 'activation_type'),
            'asr_text': asr_text,
            'topic': _get(row, 'asr', 'topics', 'model'),
            '_message_id': row['_message_id'],
            'is_target': row['_message_id'] in self._interesting_message_ids,
            'client_time': _get(row, 'speechkit_request', 'application', 'client_time'),
            'device_model': _get(row, 'speechkit_request', 'application', 'device_model'),
            'alarm_playing': _get(device_state, 'alarm_state', 'currently_playing'),
            'sound_level': '{}/{}'.format(_get(device_state, 'sound_level'), _get(device_state, 'sound_max_level')),
            'current_music_track': _get(device_state, 'music', 'currently_playing', 'track_id'),
            'is_music_player_paused': _get(device_state, 'music', 'player', 'pause'),
            'is_tv_plugged_in': _get(device_state, 'is_tv_plugged_in'),
            'voice_response_text': _get(row, 'speechkit_response', 'voice_response', 'output_speech', 'text'),
            'response_cards': cards,
            'response_directives': directives,
        }

        yield result


def main():
    args = _parse_args()

    with yt.Transaction():
        interesting_message_ids = set([r.request_id for r in yt.read_table_structured(args.requests_table, RequestRow)])
        logger.info("Found %d interesting requests", len(interesting_message_ids))

        yt.run_map(
            Mapper(interesting_message_ids),
            args.wonder_table,
            args.result_table,
        )
