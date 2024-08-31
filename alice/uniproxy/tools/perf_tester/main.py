import argparse
import getpass
import json
import multiprocessing
import os
import socket
import sys
import traceback

import tqdm
import tornado

from alice.acceptance.modules.request_generator.lib import app_presets

import alice.uniproxy.library.perf_tester.events as events
import voicetech.asr.tools.robin.lib.uniproxy as uniproxy
import voicetech.asr.tools.robin.lib.messages as messages
import voicetech.common.lib.utils as utils
import yt.wrapper as yt


logger = utils.initialize_logging(__name__)

DEFAULT_CHUNK_DURATION_MS = 200
DEFAULT_APP_PRESET = 'quasar'
DEFAULT_LANGUAGE = 'ru-RU'
DEFAULT_SPOTTER_PHRASE = 'алиса'
DEFAULT_PROCESS_ID = 'unknown_process, user = {}, host = {}'.format(getpass.getuser(), socket.getfqdn())
DEFAULT_UNIPROXY_READ_TIMEOUT_SEC = 10

# https://wiki.yandex-team.ru/voiceinfra/services/uniproxy/spotter-validation/

DEFAULT_ADVANCED_ASR_OPTIONS = {
    'utterance_silence': 900
}

DEFAULT_EXPERIMENTS = [
    'enable_e2e_eou',
    'enable_partials',
    'music_partials',
    'skip_multi_activation_check',
    'uniproxy_vins_timings',
    'vins_e2e_partials',
]

DEFAULT_APP_TYPE = 'quasar'


COLUMN_OTHER_INFO = 'other_info'
COLUMN_UTTID = 'uttid'
COLUMN_DATASET_ID = 'dataset_id'
KEY_END_OF_SPEECH = 'end_of_speech'


class StreamObserver(object):
    def __init__(self, namespace, name):
        self._namespace = namespace
        self._name = name
        self._data = None
        self._completed = False

    def on_data(self, data):
        try:
            header = data['directive']['header']
            namespace = header['namespace']
            name = header['name']

            if namespace == self._namespace and name == self._name:
                self._data = data
                self._completed = True
        except KeyError as e:
            if 'streamcontrol' not in data:
                logger.error('Failed to parse data: {} {}'.format(data, e))

    def data(self):
        return self._data

    def completed(self):
        return self._completed


class OnlineValidationObserver(StreamObserver):
    def __init__(self):
        super().__init__("Spotter", "Validation")

    def is_failed(self):
        return self.completed() and self.data()['directive']['payload']['result'] == 0


async def run_request(request, vins_observer, tts_observer, error_observer, ov_observer):
    try:
        await request.send_messages()
        while True:
            msg = await request.read_message()
            if msg is None:
                break

            if isinstance(msg, bytes):
                continue

            msg = json.loads(msg)

            for observer in [vins_observer, tts_observer, error_observer, ov_observer]:
                observer.on_data(msg)

            if error_observer.completed():
                break

            if ov_observer.is_failed():
                break

            if vins_observer.completed() and tts_observer.completed():
                break
    finally:
        request.close()


def parse_end_of_speech(row, args):
    if args.use_end_of_speech_column:
        end_of_speech = row[bytes(KEY_END_OF_SPEECH, 'utf-8')]
    else:
        other_info = row[bytes(COLUMN_OTHER_INFO, 'utf-8')]
        end_of_speech = other_info.get(bytes(KEY_END_OF_SPEECH, 'utf-8'))
    return end_of_speech


def process_row(row, args):
    uttid = row[bytes(COLUMN_UTTID, 'utf-8')].decode('utf-8')

    app_preset = app_presets.APP_PRESET_CONFIG[args.app_preset]
    request_audio = row[bytes(args.request_audio_col, 'utf-8')]
    mime = messages.FORMAT_OPUS if uniproxy.is_opus_stream_header(request_audio) else messages.FORMAT_PCM

    if args.request_audio_chunks_col:
        timings = row[bytes(args.request_audio_chunks_col, 'utf-8')]
        request_chunks = uniproxy.make_logged_chunks_from_preextracted_timings(request_audio, timings)
    else:
        request_chunks = uniproxy.make_fixed_duration_chunks(request_audio, args.chunk_duration_ms / 1000, mime == messages.FORMAT_OPUS)

    spotter_chunks=[]
    spotter_phrase = None
    if args.spotter_audio_col:
        assert args.spotter_phrase_col, "Spotter phrase col is required"
        assert args.spotter_audio_chunks_col, "Spotter audio chunks col is required"

        if spotter_phrase_bytes := row[bytes(args.spotter_phrase_col, 'utf-8')]:
            spotter_phrase = spotter_phrase_bytes.decode('utf-8')
        else:
            spotter_phrase = DEFAULT_SPOTTER_PHRASE

        if spotter_audio := row.get(bytes(args.spotter_audio_col, 'utf-8')):
            assert spotter_phrase, f"Spotter phrase is empty for uttid {uttid}"
            timings = row[bytes(args.spotter_audio_chunks_col, 'utf-8')]
            spotter_chunks = uniproxy.make_logged_chunks_from_preextracted_timings(spotter_audio, timings)
            spotter_chunks = uniproxy.extract_spotter_only_chunks_from_legacy_online_validation_session(spotter_chunks)

    retry_count = 5
    while retry_count > 0:
        try:
            request = uniproxy.Request.make(
                is_vins_voiceinput_request=True,
                url=args.url,
                auth_token=app_preset.auth_token,
                asr_topic=app_preset.asr_topic,
                experiments_list=args.experiments,
                advanced_asr_options=args.advanced_asr_options,
                mime=mime,
                request_chunks=request_chunks,
                spotter_chunks=spotter_chunks,
                lang=args.language,
                shooting_source=args.process_id,
                headers_str=args.headers,
                punctuation=args.punctuation,
                vins_app_info_dict=app_preset.application,
                oauth_token=args.oauth_token or os.environ.get('UNIPROXY_OAUTH_TOKEN'),
                vins_url=args.vins_url,
                bass_url=args.bass_url,
                spotter_phrase=spotter_phrase)

            vins_observer = StreamObserver(namespace='Vins', name='UniproxyVinsTimings')
            tts_observer = StreamObserver(namespace='TTS', name='UniproxyTTSTimings')
            error_observer = StreamObserver(namespace='System', name='EventException')
            ov_observer = OnlineValidationObserver()

            tornado.ioloop.IOLoop.current().run_sync(lambda: run_request(request, vins_observer, tts_observer, error_observer, ov_observer))
            break
        except:
            logger.error("Exception: " + traceback.format_exc())
            retry_count -= 1
            if retry_count == 0:
                logger.error("Skipping")
                return None
            else:
                logger.info("Retrying")

    if error_observer.completed():
        return error_observer.data()

    if ov_observer.is_failed():
        return ov_observer.data()

    if not vins_observer.completed():
        logger.info(f"Failed to get uniproxy timings for uttid {uttid}, request id {request.get_request_id()}")
        return None

    data = vins_observer.data()
    directive = data['directive']
    payload = directive.setdefault('payload', {})
    if tts_observer.completed():
        payload.update(tts_observer.data()['directive'].get('payload', {}))

    def add_event_to_timings(event, value):
        assert event.NAME not in payload
        if value is not None:
            payload[event.NAME] = value

    add_event_to_timings(event=events.EventRequestId, value=request.get_request_id())
    add_event_to_timings(event=events.EventEndOfSpeechSec, value=parse_end_of_speech(row, args))
    add_event_to_timings(event=events.EventUttid, value=uttid)
    add_event_to_timings(event=events.EventAppType, value=args.app_type)

    dataset_col_name = bytes(COLUMN_DATASET_ID, 'utf-8')
    payload[COLUMN_DATASET_ID] = row.get(dataset_col_name, b"").decode('utf-8')

    return data


def parse_args(args=None):
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--url',
        type=str,
        default='wss://uniproxy.alice.yandex.net/uni.ws',
        help='Uniproxy url',
        required=True)
    parser.add_argument(
        '--vins-url',
        type=str,
        help='Vins url',
        required=True)
    parser.add_argument(
        '--bass-url',
        type=str,
        default=None,
        help='Bass url')
    parser.add_argument(
        '--app-preset',
        type=str,
        default=DEFAULT_APP_PRESET,
        help='App preset used in communication with Uniproxy')
    parser.add_argument(
        '--process-id',
        type=str,
        default=DEFAULT_PROCESS_ID,
        help='Process id used to identify uniquely this shooting process')
    parser.add_argument(
        '--advanced-asr-options',
        type=json.loads,
        default=DEFAULT_ADVANCED_ASR_OPTIONS,
        help='Dict of ASR-specific experiments')
    parser.add_argument(
        '--punctuation',
        action='store_true',
        help='Punctuation flag')
    parser.add_argument(
        '--experiments',
        type=str,
        default=DEFAULT_EXPERIMENTS,
        help='List of general experiments')
    parser.add_argument(
        '--chunk-duration-ms',
        type=int,
        default=DEFAULT_CHUNK_DURATION_MS,
        help='Default chunk duration, in milliseconds, unless --logged-request-chunks-col is specified')
    parser.add_argument(
        '--uniproxy-read-timeout-sec',
        type=int,
        default=DEFAULT_UNIPROXY_READ_TIMEOUT_SEC,
        help='Timeout used to read from Uniproxy')
    parser.add_argument(
        '--language',
        type=str,
        default=DEFAULT_LANGUAGE,
        help='Language used to request Uniproxy')
    parser.add_argument(
        '--oauth-token',
        type=str,
        default='',
        help='Client app OAuth token')
    parser.add_argument(
        '--yt-table',
        type=str,
        help='Path to the YT table with data',
        required=True)
    parser.add_argument(
        '--processes',
        type=int,
        default=10,
        help='Number of parallel shooting processes')
    parser.add_argument(
        '--app-type',
        type=str,
        default=DEFAULT_APP_TYPE,
        help='App type used to tag requests')
    parser.add_argument(
        '--header', '-H',
        type=str,
        action='append',
        dest='headers',
        help='HTTP header to be added into connection request')

    parser.add_argument(
        '--request-audio-col',
        type=str,
        required=True,
        help='Column for request audio')

    parser.add_argument(
        '--request-audio-chunks-col',
        type=str,
        help='Column for request audio chunks data')

    parser.add_argument(
        '--spotter-audio-col',
        type=str,
        help='Column for spotter validation audio')

    parser.add_argument(
        '--spotter-audio-chunks-col',
        type=str,
        help='Column for spotter audio chunks data')

    parser.add_argument(
        '--spotter-phrase-col',
        type=str,
        default="",
        help='Spotter phrase col name')

    parser.add_argument(
        '--result',
        type=str,
        help='result output file (JSON) (if not given, use stdout)')

    parser.add_argument(
        '--use-end-of-speech-column',
        action='store_true',
        help='other_info.end_of_speech is used by default to extract end_of_speech time')

    return parser.parse_args(args)


def main():
    args = parse_args()
    if isinstance(args.experiments, str):
        args.experiments = args.experiments.split(',')

    # logging path where we search ffmpeg
    logger.info('PATH={}'.format(os.environ['PATH']))

    if args.headers is not None:
        args.headers = "\r\n".join(args.headers)

    logger.info('Reading voicetable...')
    pool = multiprocessing.Pool(args.processes)
    processes = [
        pool.apply_async(process_row, args=(row, args))
        for idx, row in enumerate(yt.read_table(
            yt.TablePath(args.yt_table),
            format=yt.YsonFormat(encoding=None)
        )) if not row.get(b'spotter_validation_failed', False)
    ]

    responses = [process.get() for process in tqdm.tqdm(processes)]
    responses = list(filter(None, responses))

    logger.info(f'Serializing results to JSON.... Items count {len(responses)}')
    if not args.result:
        json.dump(responses, fp=sys.stdout)
    else:
        with open(args.result, 'w') as fres:
            json.dump(responses, fp=fres)
