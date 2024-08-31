#!/usr/bin/env python
# coding: utf-8
from __future__ import unicode_literals

import argparse
import codecs
import collections
import logging
import logging.config
import json
import os

import attr
import multiprocessing
import itertools

from vins_core.dm.request import AppInfo, ReqInfo
from vins_core.utils.config import get_setting
from vins_core.utils.datetime import utcnow, timestamp_to_datetime, parse_tz
from vins_sdk.connectors import TestConnector
from vins_core.dm.session import InMemorySessionStorage, Session
from vins_core.utils.misc import gen_uuid_for_tests
from vins_api.common.vins_apps import create_app
from vins_core.utils.data import find_vinsfile

THREAD_NUM = int(get_setting('NUM_PROCS', 20))

APPS = {
    'personal_assistant': 'personal_assistant.app.PersonalAssistantApp',
    'navi_app': 'navi_app.app.NaviApp',
}

logger = logging.getLogger(__name__)


@attr.s
class LineContent(object):
    line = attr.ib()
    answer = attr.ib()
    session = attr.ib()
    source = attr.ib()


class TestUser(object):
    def __init__(self, tags, api):
        self.tags = tags
        self.token = None
        self._api = api
        self._test_user = None

        if self.tags and self._api:
            self._test_user = self._api.get_test_user(self.tags)
            self.uuid = self._test_user.uuid
            self.token = self._test_user.token
        else:
            self.uuid = str(gen_uuid_for_tests())

    def __del__(self):
        if self._api and self._test_user:
            self._api.free_test_user(self._test_user.login)


@attr.s
class VinsWrapper(object):
    connector = attr.ib()
    storage = attr.ib()
    _test_user = attr.ib(default=None)
    _vins_wrapper = None

    @classmethod
    def get(cls, appname):
        if cls._vins_wrapper:
            return cls._vins_wrapper

        storage = InMemorySessionStorage()

        logger.info('Loading model...')

        vins_app = create_app(
            appname,
            {'path': find_vinsfile(appname), 'class': APPS[appname]},
            session_storage=storage,
        )
        connector = TestConnector(vins_app=vins_app)

        cls._vins_wrapper = VinsWrapper(connector, storage)

        return cls._vins_wrapper

    @staticmethod
    def _get_app_class_name(o):
        return '{}.{}'.format(o.__module__, o.__class__.__name__)

    def get_test_user(self, tags):
        if self._test_user is None:
            api = None
            if self._get_app_class_name(self.connector.vins_app) == APPS['personal_assistant']:
                api = self.connector.vins_app.get_api()
            self._test_user = TestUser(tags, api)
        return self._test_user


def _handle_request(appname, experiments, line, app_info, device_state, test_user_tags, source):
    try:
        utterance = line.utterance
        vins_wrapper = VinsWrapper.get(appname)
        additional_options = {'bass_options': {'client_ip': '77.88.55.77'}}
        uuid = None

        if test_user_tags:
            test_user = vins_wrapper.get_test_user(test_user_tags)
            uuid = test_user.uuid
            if test_user.token:
                additional_options.update({'oauth_token': test_user.token})
        elif os.environ.get('YA_PLUS_TOKEN'):
            uuid = '791697891'
            additional_options.update({'oauth_token': os.environ['YA_PLUS_TOKEN']})

        if not experiments:
            experiments = line.experiments
        if not device_state:
            device_state = line.device_state

        client_time = timestamp_to_datetime(line.timestamp, parse_tz(line.timezone)) if line.timestamp else utcnow()
        answer = vins_wrapper.connector.handle_utterance(uuid, utterance, experiments=experiments,
                                                         device_state=device_state, text_only=False,
                                                         additional_options=additional_options,
                                                         reset_session=True, app_info=app_info,
                                                         client_time=client_time)
        req_info = ReqInfo(client_time=client_time, uuid=uuid, app_info=app_info)
        session = vins_wrapper.storage.load(appname, uuid, req_info)

        return LineContent(
            line,
            answer,
            session,
            source
        )

    except Exception as e:
        return LineContent(
            line,
            {'error': e.message},
            Session(None, None),
            source
        )


def handle_request(args):
    return _handle_request(*args)


def raw_line_processor(line_content, **kwargs):
    yield {
        'utterance': line_content.line.utterance,
        'answer': line_content.answer,
        'form': line_content.session.form.to_dict() if line_content.session.form else {},
        'intent_name': line_content.session.intent_name,
        'true_intent_name': line_content.line.true_intent,
        'mds_key': line_content.line.mds_key,
        'original_text': line_content.line.text,
        'timestamp': line_content.line.timestamp,
        'timezone': line_content.line.timezone,
        'experiments': line_content.line.experiments,
        'device_state': line_content.line.device_state,
    }


def simple_line_processor(line_content, **kwargs):
    slots = kwargs.get('slots')

    result = {
        'utterance': line_content.line.utterance,
        'text_answer': get_text_answer(line_content.answer),
        'intent_name': line_content.session.intent_name,
        'true_intent_name': line_content.line.true_intent,
        'source': line_content.source,
    }

    if slots:
        for slot in slots:
            result['form.' + slot] = None

    if line_content.session.form:
        for slot in line_content.session.form.slots:
            if slots is None or slot.name in slots:
                result['form.' + slot.name] = slot.value

    yield result


def get_directives(answer, names):
    payloads = collections.defaultdict(dict)
    for directive in answer.get('directives', []):
        directive_name = directive.get('name', '')
        if directive_name in names and directive_name not in payloads:
            payloads[directive_name] = directive.get('payload', {})
    return payloads


def video_line_processor(line_content, **kwargs):
    result = {
        'utterance': line_content.line.utterance,
        'text_answer': get_text_answer(line_content.answer),
        'intent_name': line_content.session.intent_name,
        'true_intent_name': line_content.line.true_intent,
    }

    directives = get_directives(line_content.answer, ('show_gallery', 'video_play', 'show_description'))
    gallery_items = directives['show_gallery'].get('items', [])
    show_description_item = directives['show_description']
    video_play_item = directives['video_play']

    for index, item in enumerate(gallery_items):
        sub_result = result.copy()
        sub_result['video_gallery.index'] = index
        for key, value in item.iteritems():
            sub_result['video_gallery.' + key] = value
        yield sub_result

    if show_description_item:
        sub_result = result.copy()
        for key, value in show_description_item.iteritems():
            sub_result['show_description.' + key] = value
        yield sub_result

    if video_play_item:
        sub_result = result.copy()
        tv_show_item = video_play_item.get('tv_show_item', {})
        if tv_show_item:
            for key, value in tv_show_item.iteritems():
                sub_result['video_play.tv_show_item.' + key] = value
        item = video_play_item.get('item', {})
        if item:
            for key, value in item.iteritems():
                sub_result['video_play.item.' + key] = value
        yield sub_result


PROCESSORS = {
    'raw': raw_line_processor,
    'simple': simple_line_processor,
    'video': video_line_processor,
}


def escape_special_symbols(line):
    return line.replace('\n', '\\n').replace('\r', '\\r').replace('\t', '\\t')


def serialize_value(value):
    if isinstance(value, unicode):
        return escape_special_symbols(value)
    if isinstance(value, dict):
        return json.dumps(value, ensure_ascii=False)
    return unicode(value)


def get_text_answer(answer):
    return '\n'.join(c.get('text', '') for c in answer.get('cards', []))


@attr.s
class InputLine(object):
    utterance = attr.ib()
    true_intent = attr.ib(default=None)
    experiments = attr.ib(default=None)
    device_state = attr.ib(default=None)
    timestamp = attr.ib(default=None)
    timezone = attr.ib(default=None)
    mds_key = attr.ib(default=None)
    text = attr.ib(default=None)


class FileWriter(object):
    def __init__(self, filename):
        self.filename = filename

    def write_line(self, processed_line):
        raise NotImplementedError()

    def write_lines(self, lines):
        raise NotImplementedError()

    def __enter__(self):
        self._f_out = codecs.open(self.filename, 'w', encoding='utf-8')
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._f_out.close()

    def __str__(self):
        return self.filename


class JsonFileWriter(FileWriter):
    def __init__(self, filename):
        super(JsonFileWriter, self).__init__(filename)

    def write_line(self, processed_line):
        json.dump(processed_line, self._f_out, ensure_ascii=False)
        self._f_out.write('\n')

    def write_lines(self, lines):
        for line in lines:
            self.write_line(line)


class TsvFileWriter(FileWriter):
    def __init__(self, filename):
        super(TsvFileWriter, self).__init__(filename)

    def __enter__(self):
        super(TsvFileWriter, self).__enter__()
        self._first_line = True
        return self

    def write_line(self, processed_line):
        sorted_line_items = sorted(processed_line.items())
        if self._first_line:
            self._f_out.write('\t'.join(key for key, _ in sorted_line_items))
            self._f_out.write('\n')
            self._first_line = False

        self._f_out.write('\t'.join(
            (serialize_value(value) if value is not None else ''
             for _, value in sorted_line_items)))
        self._f_out.write('\n')

    def write_lines(self, lines):
        for line in lines:
            self.write_line(line)


class FileReader(object):
    def __init__(self, filename, ignore_first_line=False):
        self.filename = filename
        self.ignore_header = ignore_first_line

    def read_lines(self):
        for line in self._f_in.readlines():
            if self._header:
                self._header = False
                continue
            line = line.strip()
            if line:
                line = line.split('\t')
                if len(line) > 1:
                    yield InputLine(utterance=line[0], true_intent=line[1])
                else:
                    yield InputLine(utterance=line[0])

    def __enter__(self):
        self._f_in = codecs.open(self.filename, 'r', encoding='utf-8')
        self._header = True & self.ignore_header
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._f_in.close()

    def __str__(self):
        return self.filename


class YtReader(object):
    def __init__(self, proxy, path):
        self.proxy = proxy
        self.path = path

    def read_lines(self):
        from yt.wrapper import YtHttpResponseError
        try:
            for row in self.client.read_table(self.path):
                query = row.get('query')
                query = query.decode('utf-8') if query else None
                yield InputLine(
                    utterance=query,
                    true_intent=row.get('true_intent'),
                    mds_key=row.get('mds_key', None),
                    text=row.get('text', None),
                    experiments=row.get('experiments'),
                    timestamp=row.get('timestamp'),
                    timezone=row.get('timezone'),
                    device_state=row.get('device_state')
                )
        except YtHttpResponseError as e:
            logger.exception('Error reading yt table: %s', e.message)

    def __enter__(self):
        from yt.wrapper import YtClient
        self.client = YtClient(proxy=self.proxy, token=get_setting('YT_TOKEN', default='', prefix=''))
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def __str__(self):
        return '%s:%s' % (self.proxy, self.path)


class YtWriter(object):
    def __init__(self, proxy, path):
        self.proxy = proxy
        self.path = path

    def write_lines(self, lines):
        self._out.extend(lines)

    def __enter__(self):
        from yt.wrapper import YtClient
        self.client = YtClient(proxy=self.proxy, token=get_setting('YT_TOKEN', default='', prefix=''))
        self._out = []
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self._out:
            self.client.write_table(self.path, self._out)

    def __str__(self):
        return '%s:%s' % (self.proxy, self.path)


def get_reader(input_file):
    if isinstance(input_file, YtPath):
        return YtReader(input_file.proxy, input_file.path)
    if input_file.endswith('.tsv'):
        return FileReader(input_file, ignore_first_line=True)
    return FileReader(input_file)


def get_writer(output_file, output_format):
    if isinstance(output_file, YtPath):
        return YtWriter(output_file.proxy, output_file.path)
    if output_format == 'tsv':
        return TsvFileWriter(output_file)
    return JsonFileWriter(output_file)


@attr.s
class IntentStatisticsCounter(object):
    intent_stats = attr.ib(default=attr.Factory(collections.Counter))
    intent_hits = attr.ib(default=attr.Factory(collections.Counter))

    def add(self, intent, true_intent):
        if not true_intent:
            return
        self.intent_stats[true_intent] += 1
        if intent == true_intent:
            self.intent_hits[true_intent] += 1

    def result(self):
        return (
            (intent, self.intent_hits[intent] / float(stat))
            for intent, stat in self.intent_stats.iteritems()
        )


def process_app_on_dataset(appname, input_files, output_file, experiments,
                           app_info=None, device_state=None, test_user_tags=None,
                           line_processor=raw_line_processor, slots=None, output_format=None):
    pool = multiprocessing.Pool(THREAD_NUM)

    intent_statistics_counter = IntentStatisticsCounter()

    with get_writer(output_file, output_format) as writer:
        for input_file in input_files:
            with get_reader(input_file) as reader:
                logger.info('Evaluating %s application on %s', appname, reader)
                source_name = str(reader)
                for line_content in pool.imap_unordered(
                    handle_request,
                    itertools.izip(
                        itertools.repeat(appname),
                        itertools.repeat(experiments),
                        reader.read_lines(),
                        itertools.repeat(app_info),
                        itertools.repeat(device_state),
                        itertools.repeat(test_user_tags),
                        itertools.repeat(source_name),
                    )
                ):
                    if line_content.answer.get('error'):
                        logger.warning(
                            'Failed to process "%s": %s',
                            line_content.line.utterance,
                            line_content.answer.get('error')
                        )
                    logger.info('— %s', line_content.line.utterance)
                    logger.info('— %s', get_text_answer(line_content.answer))
                    logger.info('-----------------------------')

                    intent_statistics_counter.add(line_content.session.intent_name, line_content.line.true_intent)

                    writer.write_lines(line_processor(line_content, slots=slots))

    for intent_name, stat in intent_statistics_counter.result():
        logger.info('%s accuracy: %s', intent_name, stat)


@attr.s(frozen=True)
class YtPath(object):
    proxy = attr.ib()
    path = attr.ib()


def get_path(path_with_scheme):
    if not path_with_scheme:
        return path_with_scheme
    parts = path_with_scheme.split('://', 1)
    if len(parts) == 1:
        return parts[0]
    scheme, path = parts
    if scheme in ('hahn', 'banach'):
        return YtPath(scheme, '//' + path)
    return path_with_scheme


FORMATS = {
    'json',
    'tsv'
}


def main():
    parser = argparse.ArgumentParser(add_help=True, description='Process all strings in the input files as if they '
                                                                'were first utterances of the dialog. Answers of the '
                                                                'specified application along with intent and slot '
                                                                'filling information are stored either to the'
                                                                'specified output file or to different files '
                                                                '(file <input_file_name>_answers.csv> '
                                                                'will be created for each input)')
    parser.add_argument(
        '--app', metavar='APP', choices=APPS, required=True, help='application name')
    parser.add_argument(
        '--input-files', metavar='FILE', nargs='+', required=True,
        help='input datasets, file or yt table with proxy passed as scheme, e.g. hahn://path/to/table')
    parser.add_argument(
        '--output-file', metavar='FILE', required=True,
        help='output file with results, file or yt table with proxy passed as scheme, e.g. hahn://path/to/table')
    parser.add_argument(
        '--experiments', metavar='EXP', nargs='+', required=False, help='experiment flags')
    parser.add_argument(
        '--device-state', metavar='JSON', required=False, help='json with device state')
    parser.add_argument(
        '--app-info', metavar='JSON', required=False, help='json with app info')
    parser.add_argument(
        '--test-user-tags', metavar='TAG', nargs='+', required=False, help='test user tags')
    parser.add_argument(
        '--processor', metavar='FUNC', choices=PROCESSORS, default='simple', required=False,
        help='function to process answer')
    parser.add_argument(
        '--format', choices=FORMATS, dest='output_format', default='json', required=False, help='output file format')
    parser.add_argument(
        '--slots', metavar='SLOT', nargs='+', required=False, help='form.slots to output')

    parser.add_argument(
        '--file-log', action='store_true', dest='should_log_to_file', default=False)
    parser.add_argument(
        '--logging-level',
        metavar='LOG',
        choices=('DEBUG', 'WARNING', 'TRACE', 'INFO', 'ERROR'),
        required=False, help='logging level',
        default='WARNING'
    )
    args = parser.parse_args()

    log_handlers = {
        'console': {
            'class': 'logging.StreamHandler',
            'level': 'INFO',
            'formatter': 'standard',
        }
    }
    loggers = ['console']

    if args.should_log_to_file:
        log_handlers['file'] = {
            'level': 'DEBUG',
            'class': 'logging.FileHandler',
            'filename': 'process_app_on_dataset.log'
        }
        loggers.append('file')

    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': log_handlers,
        'loggers': {
            '': {
                'handlers': loggers,
                'level': args.logging_level,
                'propagate': True,
            },
        },
    })

    app_info = None
    device_state = None
    if args.app_info:
        app_info_json = json.loads(args.app_info)
        app_info = AppInfo(
            app_id=app_info_json.get('app_id', 'com.yandex.vins.tests'),
            app_version=app_info_json.get('app_version', '0.0.1'),
            os_version=app_info_json.get('os_version', '1'),
            platform=app_info_json.get('platform', 'unknown'),
        )

    if args.device_state:
        device_state = json.loads(args.device_state)

    process_app_on_dataset(
        appname=args.app,
        input_files=(get_path(input_file) for input_file in args.input_files),
        output_file=get_path(args.output_file),
        experiments=args.experiments,
        app_info=app_info,
        device_state=device_state,
        test_user_tags=args.test_user_tags,
        line_processor=PROCESSORS[args.processor],
        output_format=args.output_format,
        slots=args.slots
    )


if __name__ == "__main__":
    main()
