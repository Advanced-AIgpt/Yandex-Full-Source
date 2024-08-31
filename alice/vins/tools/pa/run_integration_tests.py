# coding: utf-8

from __future__ import unicode_literals

import argparse
import json
import shutil
import sys
import tempfile
from datetime import datetime

import attr
import codecs
import os
import pytz
from requests.exceptions import RequestException
import color

from personal_assistant.testing_framework import (
    load_testcase, check_response, convert_vins_response_for_tests,
    parse_placeholders,
)
from vins_core.ext.s3 import S3Bucket
from vins_core.ext.speechkit_api import SpeechKitTestAPI
from vins_core.utils.config import get_setting
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.utils.data import safe_remove
from vins_core.dm.response import VinsResponse
from personal_assistant.api.personal_assistant import PersonalAssistantAPI, PersonalAssistantAPIError

import rtlog


def colored(s, c):
    if sys.stdout.isatty():
        return color.colored(s, c)
    return s


LOCATION = {
    'lon': 37.587937,
    'lat': 55.733771
}  # Yandex Office

UNIPROXY_VINS_SESSIONS_FLAG = 'uniproxy_vins_sessions'

test_user_api = PersonalAssistantAPI()


@attr.s
class TestDialogTurn(object):
    request = attr.ib()
    response = attr.ib(default=None)
    is_match = attr.ib(default=None)
    exception = attr.ib(default=None)
    setrace_reqid = attr.ib(default=None)


class ResponseDumper(object):
    def dump(self, test_case_name, app_info, dialog_turns, is_flaky, is_xfail, successful_attempt):
        raise NotImplementedError

    def skip(self, test_case_name):
        pass

    def close(self):
        pass


class ResponseDumperList(ResponseDumper):
    def __init__(self):
        super(ResponseDumper, self).__init__()
        self._dumpers = []

    def append(self, dumper):
        self._dumpers.append(dumper)

    def dump(self, *args, **kwargs):
        for dumper in self._dumpers:
            dumper.dump(*args, **kwargs)

    def skip(self, *args, **kwargs):
        for dumper in self._dumpers:
            dumper.skip(*args, **kwargs)

    def close(self, *args, **kwargs):
        for dumper in self._dumpers:
            dumper.close(*args, **kwargs)


class JsonResponseDumperBase(ResponseDumper):
    def __init__(self):
        self._json = {
            'last_updated': str(datetime.now()),
            'tests': []
        }

    @property
    def json(self):
        return self._json

    def dump(self, test_case_name, app_info, dialog_turns, is_flaky, is_xfail, successful_attempt):
        if is_flaky:
            # Do not log flaky tests
            return

        success = all(t.is_match for t in dialog_turns)
        self._json['tests'].append({
            'name': test_case_name,
            'app_info': app_info,
            'success': success or is_xfail,
            'successful_attempt': successful_attempt,
            'turns': [
                {
                    'request': dt.request and unicode(dt.request),
                    'response': dt.response and unicode(dt.response),
                    'success': dt.is_match,
                    'exception': dt.exception and unicode(dt.exception),
                    'setrace_reqid': dt.setrace_reqid and unicode(dt.setrace_reqid),
                } for dt in dialog_turns
            ]
        })


class FileJsonResponseDumper(JsonResponseDumperBase):
    def __init__(self, filename):
        super(FileJsonResponseDumper, self).__init__()
        self._filename = filename

    def close(self):
        with codecs.open(self._filename, 'w', 'utf-8') as output_file:
            json.dump(self.json, output_file, indent=4, ensure_ascii=False)


class S3JsonResponseDumper(JsonResponseDumperBase):
    def __init__(self, s3_key):
        super(S3JsonResponseDumper, self).__init__()
        self._s3_key = s3_key
        self._s3 = S3Bucket(get_setting('S3_ACCESS_KEY_ID'), get_setting('S3_SECRET_ACCESS_KEY'))

    def close(self):
        json_dump = json.dumps(self.json, ensure_ascii=False, indent=4)
        self._s3.put(self._s3_key, json_dump)


class HtmlReportResponseDumper(JsonResponseDumperBase):
    def __init__(self, report_dir):
        super(HtmlReportResponseDumper, self).__init__()
        self._report_dir = report_dir

    def close(self):
        # TODO: fix me!!!!
        viewer_dir = os.path.join('integration_test_viewer')
        safe_remove(self._report_dir)
        shutil.copytree(viewer_dir, self._report_dir)
        safe_remove(os.path.join(self._report_dir, '.git'))
        data_filename = os.path.join(self._report_dir, 'data.js')
        with codecs.open(data_filename, 'w', 'utf-8') as output_file:
            output_file.write('data = ')
            json.dump(self.json, output_file, indent=4, ensure_ascii=False)
            output_file.write(';')


class ConsoleResponseDumper(ResponseDumper):
    def __init__(self):
        self._successful = 0
        self._successful_or_flaky = 0
        self._total = 0
        self._real_flaky = 0

    def dump(self, test_case_name, app_info, dialog_turns, is_flaky, is_xfail, successful_attempt):
        print colored(
            '%s%s%s%s' % (
                test_case_name,
                ' (xfail)' if is_xfail else '',
                ' (flaky)' if is_flaky else '',
                ' (%d unsuccesfull attempts)' % successful_attempt if successful_attempt else ''
            ),
            'magenta' if is_flaky else 'red' if is_xfail else 'yellow'
        )
        print 'APP_INFO:', json.dumps(app_info, indent=2)

        for dialog_turn in dialog_turns:
            check_box = colored('\u2713', 'green') if dialog_turn.is_match else colored('x', 'red')
            print check_box, '-', dialog_turn.request
            print ('  - %s' % dialog_turn.response).replace('\n', '\n    ')
            if dialog_turn.exception:
                try:
                    print ' ', colored(dialog_turn.exception, 'red')
                except Exception:
                    print ' ', colored(str(type(dialog_turn.exception)), 'red')
            if dialog_turn.setrace_reqid:
                print ' ', colored('(setrace: %s)' % dialog_turn.setrace_reqid, 'blue')

        success = all(t.is_match for t in dialog_turns)
        if success:
            if is_xfail:
                print '%s' % colored('UNEXPECTED SUCCESS', 'red')
                self._successful_or_flaky += 1 if is_flaky else 0
            else:
                print '%s' % colored('SUCCESS', 'green')
                self._successful += 1
                self._successful_or_flaky += 1
        else:
            if is_xfail:
                print '%s' % colored('FAIL AS EXPECTED', 'green')
                self._successful += 1
                self._successful_or_flaky += 1
            else:
                self._successful_or_flaky += 1 if is_flaky else 0
                print '%s' % colored('FAIL', 'red')

        print

        self._total += 1
        self._real_flaky += 1 if successful_attempt else 0

    def close(self):
        if self._total == self._successful:
            print colored('All %d tests were successful' % self._total, 'green')
        elif self._total == self._successful_or_flaky:
            print colored(
                '%d out of %d tests were successful, %d flaky tests failed' % (
                    self._successful,
                    self._total,
                    self._successful_or_flaky - self._successful
                ),
                'yellow'
            )
        else:
            print colored(
                '%d out of %d tests were successful, '
                '%d flaky tests failed, %d normal tests failed' % (
                    self._successful,
                    self._total,
                    self._successful_or_flaky - self._successful,
                    self._total - self._successful_or_flaky
                ),
                'red'
            )

        if self._real_flaky:
            print colored('%d tests were flaky during testing' % self._real_flaky, 'magenta')


def process_test_case(speechkit_client, dialog_test_data, dumpers, retries, should_trace, additional_experiments=None):
    tz = pytz.timezone('Europe/Moscow')

    successful_attempt = None
    dialog_turns = None
    is_xfail = dialog_test_data.status == 'xfail'
    is_flaky = dialog_test_data.status == 'flaky'

    for attempt in xrange(retries):
        dialog_turns = []
        test_user = None
        uuid = None
        additional_options = dialog_test_data.additional_options
        if dialog_test_data.user_tags:
            try:
                test_user = test_user_api.get_test_user(dialog_test_data.user_tags)

                bass_options = {'client_ip': test_user.client_ip}
                bass_options.update(additional_options.get('bass_options', {}))

                additional_options['oauth_token'] = test_user.token
                additional_options['bass_options'] = bass_options
                uuid = test_user.uuid
            except PersonalAssistantAPIError as e:
                dialog_turns.append(TestDialogTurn(request=dialog_test_data.dialog[0], exception=e))
                continue

        if not test_user:
            uuid = str(gen_uuid_for_tests())

        try:
            full_match = True
            first_utterance = True
            session = None
            for utterance_test_data in dialog_test_data.dialog:
                dt = datetime.now(tz)

                try:
                    experiments = utterance_test_data.experiments
                    if additional_experiments is not None:
                        for k, v in additional_experiments.iteritems():
                            experiments[k] = v

                    request_label = None
                    setrace_reqid = None
                    if should_trace:
                        request_label = 'vins_integration_tests: {0} {1}'.format(dialog_test_data.name, utterance_test_data.request)
                        rtlog.thread_local.begin_request(None)
                        setrace_reqid = rtlog.thread_local.get_token()
                        if setrace_reqid and setrace_reqid.count('$') == 2:
                            setrace_reqid = setrace_reqid.split('$')[1]

                    api_response = speechkit_client.request(
                        uuid, dt, utterance_test_data.event, dialog_test_data.geo_info or LOCATION,
                        experiments=experiments, app_info=dialog_test_data.app_info, string_response=False,
                        device_state=dialog_test_data.device_state, additional_options=additional_options,
                        reset_session=first_utterance, dialog_id=utterance_test_data.dialog_id,
                        lang=utterance_test_data.lang, request_label=request_label, session=session
                    )
                except RequestException as e:
                    dialog_turns.append(TestDialogTurn(request=utterance_test_data.request, exception=e, setrace_reqid=setrace_reqid))
                    full_match = False
                    break
                finally:
                    if should_trace:
                        rtlog.thread_local.end_request()

                first_utterance = False
                vins_response = VinsResponse()
                SpeechKitTestAPI.deserialize_result(api_response, vins_response)
                response_for_tests = convert_vins_response_for_tests(vins_response.to_dict())
                session = vins_response.sessions.get('', None)

                is_match, error = check_response(utterance_test_data, response_for_tests)

                dialog_turns.append(
                    TestDialogTurn(
                        request=utterance_test_data.request,
                        response=response_for_tests.text,
                        is_match=is_match,
                        exception=error,
                        setrace_reqid=setrace_reqid
                    )
                )

                if not is_match:
                    full_match = False
                    break

            if full_match or is_xfail:
                successful_attempt = attempt
                break
        finally:
            if test_user:
                test_user_api.free_test_user(test_user.login)

    dict_app_info = {}
    if dialog_test_data.app_info:
        dict_app_info = dialog_test_data.app_info.to_dict()

    dumpers.dump(
        dialog_test_data.name,
        dict_app_info,
        dialog_turns,
        is_flaky,
        is_xfail,
        successful_attempt
    )

    return successful_attempt is not None


def main():
    def positive_int(value):
        int_value = int(value)
        if int_value <= 0:
            raise argparse.ArgumentTypeError("%s is an invalid positive int value" % value)
        return int_value

    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--vins-url', metavar='URL', dest='vins_url', required=True,
                        help='A speechkit API url to test')
    parser.add_argument('--bass-url', metavar='URL', dest='bass_url', required=False,
                        help='Allows to overwrite BASS url')
    parser.add_argument('--req-wizard-url', metavar='URL', dest='req_wizard_url', required=False,
                        help='Allows to overwrite Request Wizard url')
    parser.add_argument('--json-dump', metavar='FILE', dest='json_dump', required=False,
                        help='Dumps test run results to a viewer-compatible json file')
    parser.add_argument('--s3-dump', metavar='KEY', dest='s3_dump', required=False,
                        help='Dumps test run results in a viewer-compatible json format to a specified S3 key')
    parser.add_argument('--html-report', metavar='DIR', dest='html_report', required=False,
                        help='Creates a HTML test run report in the specified directory.')
    parser.add_argument('--voice', dest='voice', action='store_true',
                        help='Issue queries as voice instead of text')
    parser.add_argument('--retries', dest='retries', type=positive_int, default=1,
                        help='The number of retries for a failed test, allows to detect flaky tests')
    parser.add_argument('--trace', dest='should_trace', action='store_true', default=False,
                        help='Trace requests by SETrace')
    parser.add_argument('--fast', dest='is_fast_mode', action='store_true', default=False,
                        help='Fast mode: process only one platform for each test case.')
    parser.add_argument(
        "--placeholders", action="store", default='',
        help=(
            "Allow replacing placeholders with provided values. "
            "Format: `key1:any value1;key2:any value2`"
        )
    )
    parser.add_argument(
        "--test-prefix", action="store", default='',
        help="Allows to choose test cases which names start with specified prefix"
    )
    parser.add_argument('--experiments', dest='experiments', default='',
                        help='Allows to add extra experiment flags. Format: `key1:any value1;key2:any value2`')

    args = parser.parse_args()

    rtlog_path = None
    if args.should_trace:
        rtlog_fd, rtlog_path = tempfile.mkstemp('_test_rtlog')
        os.close(rtlog_fd)
        rtlog.activate(rtlog_path, 'vins_integration_tests')

    placeholders = parse_placeholders(args.placeholders)
    additional_experiments = parse_placeholders(args.experiments)
    if UNIPROXY_VINS_SESSIONS_FLAG not in additional_experiments:
        print colored('Warning! Force adding experiment flag: %s' % UNIPROXY_VINS_SESSIONS_FLAG, 'yellow')
        additional_experiments[UNIPROXY_VINS_SESSIONS_FLAG] = 1

    speechkit_client = SpeechKitTestAPI(
        vins_url=args.vins_url, bass_url=args.bass_url, req_wizard_url=args.req_wizard_url, as_voice=args.voice)

    dumpers = ResponseDumperList()
    dumpers.append(ConsoleResponseDumper())
    if args.json_dump:
        dumpers.append(FileJsonResponseDumper(args.json_dump))
    if args.s3_dump:
        dumpers.append(S3JsonResponseDumper(args.s3_dump))
    if args.html_report:
        dumpers.append(HtmlReportResponseDumper(args.html_report))

    successful = 0
    successful_or_flaky = 0
    total = 0
    for dialog_test_data in load_testcase('integration_data', placeholders, args.is_fast_mode):
        if (
                dialog_test_data.status == 'skip' or
                not dialog_test_data.name.startswith(args.test_prefix)
        ):
            dumpers.skip(dialog_test_data.name)
            continue

        success = process_test_case(speechkit_client, dialog_test_data, dumpers, args.retries, args.should_trace,
                                    additional_experiments=additional_experiments)
        total += 1
        successful += 1 if success else 0
        successful_or_flaky += 1 if success or dialog_test_data.status == 'flaky' else 0

    dumpers.close()
    if rtlog_path:
        os.remove(rtlog_path)

    if total != successful and total != successful_or_flaky:
        return 1

    return 0


def do_main():
    sys.exit(main())


if __name__ == '__main__':
    do_main()
