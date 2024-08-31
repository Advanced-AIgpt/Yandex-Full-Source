# coding: utf-8
import argparse
import json
import logging
import time
from collections import defaultdict

import codecs
import os
import requests
import attr

from personal_assistant.api.personal_assistant import PersonalAssistantAPI
from personal_assistant.app import PersonalAssistantApp
from personal_assistant.testing_framework import load_testcase, get_vins_response, check_response, parse_placeholders, \
    turn_into_stub_filename
from vins_core.utils.data import find_vinsfile
from vins_sdk.connectors import TestConnector
from vins_core.utils.misc import gen_uuid_for_tests

logger = logging.getLogger(__name__)

test_user_api = PersonalAssistantAPI()


def spec_matched(name, specs):
    if not specs:
        return True

    return name.startswith(tuple(specs))


@attr.s
class DumpElement(object):
    intent_before_request = attr.ib()
    bass_response = attr.ib()


class DumpDataAdapter(requests.adapters.HTTPAdapter):
    def __init__(self, dump_elements, **kwargs):
        self._dump_elements = dump_elements
        super(DumpDataAdapter, self).__init__(**kwargs)

    def build_response(self, req, resp):
        response = super(DumpDataAdapter, self).build_response(req, resp)
        response.raise_for_status()
        request = json.loads(req.body)
        self._dump_elements.append(DumpElement(request.get('form', {}).get('name', ''), response.json()))

        return response


class BassApi(PersonalAssistantAPI):
    TIMEOUT = 10

    def __init__(self, url, *args, **kwargs):
        super(BassApi, self).__init__(*args, **kwargs)
        self._bass_url = url

    def _get_url_prefix(self, req_info, balancer_type):
        return self._bass_url


class App(PersonalAssistantApp):
    def __init__(self, bass_url, *args, **kwargs):
        super(App, self).__init__(*args, **kwargs)
        self._bass_url = bass_url
        self._pa_api = BassApi(bass_url)

    def mount(self, adapter):
        self._pa_api.http.mount(self._bass_url, adapter)


class Dumper(object):
    def __init__(self, app, directory):
        self._app = app
        self._path = directory
        if not os.path.exists(directory):
            raise ValueError('Dump directory %s does not exist' % directory)

    def dump_dialog(self, dialog_test_data):
        test_user = None
        if dialog_test_data.user_tags:
            test_user = test_user_api.get_test_user(dialog_test_data.user_tags)
            dialog_test_data.additional_options['test_user_oauth_token'] = test_user.token
            uuid = test_user.uuid
        else:
            test_user = None
            uuid = str(gen_uuid_for_tests())

        dump_elements = []

        adapter = DumpDataAdapter(dump_elements)
        self._app._vins_app.mount(adapter)

        try:
            logger.info('Launching test %s (status %s)', dialog_test_data.get_full_test_name(), dialog_test_data.status)
            tm = time.time()

            success = True
            problems = []
            for utterance_test_data in dialog_test_data.dialog:
                response = get_vins_response(self._app, uuid, dialog_test_data, utterance_test_data)

                is_match, problem = check_response(utterance_test_data, response)
                success = success and is_match
                if problem:
                    problems.append(problem)

            is_xfail = dialog_test_data.status == 'xfail'

            if success != is_xfail:
                logger.info('Saving %s test responses to file', dialog_test_data.get_full_test_name())
                fname = os.path.join(self._path,
                                     '%s.stub' % turn_into_stub_filename(dialog_test_data.get_full_test_name()))
                with codecs.open(fname, 'w', encoding='utf-8') as f:
                    f.write('%s\n' % tm)
                    for dump_element in dump_elements:
                        f.write('%s\n' % dump_element.intent_before_request)
                        json.dump(dump_element.bass_response, f, ensure_ascii=False)
                        f.write('\n')

                status = 'xfailed' if is_xfail else 'passed'
            else:
                logger.error("Test didn't behave as expected. Problems detected:\n  %s", '\n  '.join(problems))
                status = 'xpassed' if is_xfail else 'failed'

        except Exception:
            logger.exception(
                'Unhandled exception during stubs downloading for test %s',
                dialog_test_data.get_full_test_name()
            )
            status = 'exception'
        finally:
            adapter.close()
            if test_user:
                test_user_api.free_test_user(test_user.login)

        return status


def create_app(bass_url):
    app = App(bass_url, vins_file=find_vinsfile('personal_assistant'))
    return TestConnector(vins_app=app)


def main():
    config_dir = os.path.dirname(find_vinsfile('personal_assistant'))
    stubs_dir = os.path.join(config_dir, '..', 'tests', 'integration_data', 'stubs')

    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--test-cases', metavar='SPEC', nargs='+',
                        help=(
                            'Test case spec. Format: `test_file::test_name`'
                            ' or `test_file`. If omitted stubs will be created'
                            ' for all integration tests'
                        ))
    parser.add_argument('--bass-url', metavar='URL',
                        default='',
                        help='BASS URL, optional')
    parser.add_argument('--dump-dir', metavar='DIR',
                        default=stubs_dir,
                        help='Directory for BASS responses dump')
    parser.add_argument(
        "--placeholders", action="store", default='',
        help=(
            "Allow replacing placeholders with provided values. "
            "Format: `key1:any value1;key2:any value2`"
        )
    )

    args = parser.parse_args()

    bass_url = args.bass_url
    placeholders = parse_placeholders(args.placeholders)

    if not bass_url:
        from personal_assistant.api.personal_assistant import SLOW_QUERY_BASS_API_URL
        bass_url = SLOW_QUERY_BASS_API_URL

    app = create_app(bass_url)

    dumper = Dumper(app, args.dump_dir)
    stats = defaultdict(list)

    processed_tests = set()
    for dialog_test_data in load_testcase('integration_data', placeholders):
        if spec_matched(dialog_test_data.get_full_test_name(), args.test_cases):
            processed_tests.add(dialog_test_data.get_full_test_name())
            if dialog_test_data.status == 'skip':
                logger.info('skipping %s', dialog_test_data.get_full_test_name())
                continue

            real_status = dumper.dump_dialog(dialog_test_data)
            stats[real_status].append(dialog_test_data.get_full_test_name())

    # print stats
    logger.info('Statistics:')
    logger.info('\ttotal: %s', sum(map(len, stats.itervalues())))
    logger.info('\tpassed: %s', len(stats['passed']))
    logger.info('\txfailed: %s', len(stats['xfailed']))
    logger.info('\tfailed: %s', len(stats['failed']))
    logger.info('\txpassed: %s', len(stats['xpassed']))
    logger.info('\texception: %s', len(stats['exception']))

    logger.info('Stubs were not updated for:')

    if stats['failed']:
        logger.info('\t - failed tests:')
        for test_name in stats['failed']:
            logger.info('\t\t %s', test_name)

    if stats['xpassed']:
        logger.info('\t - xpassed tests:')
        for test_name in stats['xpassed']:
            logger.info('\t\t %s', test_name)

    if stats['exception']:
        logger.info('\t - exception tests:')
        for test_name in stats['exception']:
            logger.info('\t\t %s', test_name)

    for stub_filename in os.listdir(args.dump_dir):
        stub_name = os.path.basename(stub_filename)
        stub_name, _ = os.path.splitext(stub_name)

        if spec_matched(stub_name, args.test_cases) and stub_name not in processed_tests:
            logger.info('Redundant stub detected: %s', stub_name)


def do_main():
    logging.basicConfig(level=logging.INFO)
    logging.getLogger('dialog_history').setLevel(logging.CRITICAL)
    logging.getLogger('vins_core').setLevel(logging.WARNING)
    main()


if __name__ == '__main__':
    do_main()
