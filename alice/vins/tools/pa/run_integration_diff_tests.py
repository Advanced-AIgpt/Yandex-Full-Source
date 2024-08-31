# coding: utf-8

from __future__ import unicode_literals

import argparse
import copy
import json
import sys
import tempfile
from datetime import datetime, timedelta

import attr
import codecs
import os
import pytz
import color

from personal_assistant.testing_framework import (
    load_testcase, check_response, convert_vins_response_for_tests,
    parse_placeholders,
)
from vins_core.ext.s3 import S3Bucket
from vins_core.ext.speechkit_api import SpeechKitTestAPI
from vins_core.utils.config import get_setting
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.dm.response import VinsResponse
from personal_assistant.api.personal_assistant import PersonalAssistantAPI, PersonalAssistantAPIError

import rtlog


def to_unicode(data):
    if data is None:
        return ''
    try:
        if type(data) is str:
            return unicode(data, 'utf8')
        return '%s' % data
    except Exception:
        return unicode(type(data))


# Result of test case differential testing
OK = 'OK'           # both versions (v1 and v2) are successful
IGNORE = 'IGNORE'   # both versions (v1 and v2) are broken
FLAKY = 'FLAKY'     # uncertain results, test case is too flaky
FIXED = 'FIXED'     # sure progress: v2 has much less errors than v1
FAIL = 'FAIL'       # sure regress: v2 has much more errors than v1

STATUS_COLORS = {
    OK: 'green',
    IGNORE: 'magenta',
    FLAKY: 'magenta',
    FIXED: 'green',
    FAIL: 'red',
}

LOCATION = {
    'lon': 37.587937,
    'lat': 55.733771
}  # Yandex Office

UNIPROXY_VINS_SESSIONS_FLAG = 'uniproxy_vins_sessions'

test_user_api = PersonalAssistantAPI()


@attr.s
class DialogTurnResult(object):
    time_begin = attr.ib(default=None)
    time_end = attr.ib(default=None)
    request = attr.ib(default=None)
    response = attr.ib(default=None)
    setrace_reqid = attr.ib(default=None)
    error = attr.ib(default=None)
    is_success = attr.ib(default=False)
    session = attr.ib(default=None)


@attr.s
class DialogAttemptResult(object):
    initialization_error = attr.ib(default=None)
    turns = attr.ib(default=[])
    is_success = attr.ib(default=False)


@attr.s
class StatisticsOfAttempts(object):
    count = attr.ib(default=0)
    fail_v1 = attr.ib(default=0)
    fail_v2 = attr.ib(default=0)
    success_v1 = attr.ib(default=0)
    success_v2 = attr.ib(default=0)


@attr.s
class TestCaseResult(object):
    name = attr.ib(default=None)
    app_info = attr.ib(default=None)
    attempts_v1 = attr.ib(default=[])
    attempts_v2 = attr.ib(default=[])
    statistics = attr.ib(default=StatisticsOfAttempts())
    status = attr.ib(default=None)


@attr.s
class TestSuiteStatistics(object):
    ok = attr.ib(default=0)
    ignore = attr.ib(default=0)
    flaky = attr.ib(default=0)
    fixed = attr.ib(default=0)
    fail = attr.ib(default=0)
    total = attr.ib(default=0)

    def add_test_case(self, status):
        self.ok += status == OK
        self.ignore += status == IGNORE
        self.flaky += status == FLAKY
        self.fixed += status == FIXED
        self.fail += status == FAIL
        self.total += 1


class Dumper(object):
    def dump(self, test_case_result):
        raise NotImplementedError

    def finalize(self):
        pass


class DumperList(Dumper):
    def __init__(self):
        super(Dumper, self).__init__()
        self._dumpers = []

    def append(self, dumper):
        self._dumpers.append(dumper)

    def dump(self, *args, **kwargs):
        for dumper in self._dumpers:
            dumper.dump(*args, **kwargs)

    def finalize(self, *args, **kwargs):
        for dumper in self._dumpers:
            dumper.finalize(*args, **kwargs)


class JsonDumperBase(Dumper):
    def __init__(self, flushing_period=None):
        self._statistics = TestSuiteStatistics()
        self._json = {
            'last_updated': to_unicode(datetime.now()),
            'statistics': attr.asdict(self._statistics),
            'tests': []
        }
        self._flushing_period = flushing_period
        if flushing_period:
            self._time_of_last_flush = datetime.now()

    @property
    def json(self):
        return self._json

    def flush(self):
        pass

    def dump(self, test_case):
        self._statistics.add_test_case(test_case.status)
        self._json['statistics'] = attr.asdict(self._statistics),
        self._json['tests'].append(attr.asdict(test_case))
        if self._flushing_period is not None and datetime.now() - self._time_of_last_flush > self._flushing_period:
            self.flush()
            self._time_of_last_flush = datetime.now()


class FileJsonDumper(JsonDumperBase):
    def __init__(self, filename, flushing_period=None):
        super(FileJsonDumper, self).__init__(flushing_period=flushing_period)
        self._filename = filename

    def flush(self):
        with codecs.open(self._filename, 'w', 'utf-8') as output_file:
            json.dump(self.json, output_file, indent=2, ensure_ascii=False, encoding='utf8', sort_keys=True)

    def finalize(self):
        self.flush()


class S3JsonDumper(JsonDumperBase):
    def __init__(self, s3_key):
        super(S3JsonDumper, self).__init__()
        self._s3_key = s3_key
        self._s3 = S3Bucket(get_setting('S3_ACCESS_KEY_ID'), get_setting('S3_SECRET_ACCESS_KEY'))

    def flush(self):
        json_dump = json.dumps(self.json, ensure_ascii=False, indent=2)
        self._s3.put(self._s3_key, json_dump)

    def finalize(self):
        self.flush()


class ReportDumper(Dumper):
    def __init__(self, verbose=False, filename=None, summary_only=False):
        self._verbose = verbose
        self._summary_only = summary_only
        self._file = codecs.open(filename, 'w', 'utf-8') if filename else None
        self._statistics = TestSuiteStatistics()
        self._sure_diff_report = []

    def dump(self, test_case):
        self._statistics.add_test_case(test_case.status)
        self._print_verbose(test_case)
        self._print_brief(test_case)

    def _print_verbose(self, test_case):
        if self._summary_only or (not self._verbose and test_case.status not in {FAIL, FIXED}):
            return
        self._print('')
        self._print(test_case.name, color='yellow')
        self._print('app_info:', json.dumps(test_case.app_info, indent=2), indent=2)
        self._print_attempts('v1', test_case, test_case.attempts_v1)
        self._print_attempts('v2', test_case, test_case.attempts_v2)
        s = test_case.statistics
        self._print('  Statistics of attempts:')
        self._print('    v1: %d failed, %d successful, %d total' % (s.fail_v1, s.success_v1, s.count))
        self._print('    v2: %d failed, %d successful, %d total' % (s.fail_v2, s.success_v2, s.count))

    def _print_attempts(self, version, test_case, attempts):
        self._print(version, 'attempts:', indent=2)
        for i, attempt in enumerate(attempts):
            self._print_attempt(test_case, attempt, i)

    def _print_attempt(self, test_case, attempt, attempt_index):
        self._print('attempt %d:' % attempt_index, indent=4)
        fail_color = 'red' if test_case.status == FAIL else 'magenta'
        if attempt.initialization_error:
            self._print(attempt.initialization_error, indent=6, color=fail_color)
        for turn in attempt.turns:
            check_box = self._colored('\u2713', 'green') if turn.is_success else self._colored('x', fail_color)
            self._print(check_box, '-', turn.request, indent=10, first_line_indent=-4)
            self._print('-', turn.response, indent=10, first_line_indent=-2)
            if turn.error:
                self._print(turn.error, indent=10, color=fail_color)
            if turn.setrace_reqid:
                self._print('(setrace: %s)' % turn.setrace_reqid, indent=10, color='blue')

    def _print_brief(self, test_case):
        s = test_case.statistics
        line = ''
        line += self._colored(test_case.status.ljust(6), STATUS_COLORS.get(test_case.status, 'white'))
        line += ' %d/%d->%d/%d ' % (s.success_v1, s.count, s.success_v2, s.count)
        line += self._colored(test_case.name, 'yellow')
        if not self._summary_only:
            self._print(line)
        if test_case.status in {FAIL, FIXED}:
            self._sure_diff_report.append(line)

    def finalize(self):
        self._print_final_report()
        if self._file:
            self._file.close()

    def _print_final_report(self):
        s = self._statistics
        self._print('Summary:')
        self._print('  OK:    %5d  # both versions (v1 and v2) are successful' % s.ok)
        self._print('  IGNORE:%5d  # both versions (v1 and v2) are broken' % s.ignore)
        self._print('  FLAKY: %5d  # uncertain results, test case is too flaky' % s.flaky)
        self._print('  FIXED: %5d  # sure progress: v2 has much less errors than v1' % s.fixed)
        self._print('  FAIL:  %5d  # sure regress: v2 has much more errors than v1' % s.fail)
        self._print('  Total: %5d' % s.total)
        if self._sure_diff_report:
            self._print('Test cases with sure diff:')
            for line in self._sure_diff_report:
                self._print(line, indent=2)
        if s.fail == 0:
            self._print('All tests are successful or flaky', color='green')
        else:
            self._print('%d tests are failed!' % s.fail, color='red')

    def _print(self, *args, **kwargs):
        text = self._format_paragraph(*args, **kwargs)
        if self._file:
            self._file.write(text + '\n')
        else:
            print text

    def _format_paragraph(self, *args, **kwargs):
        text = ' '.join([to_unicode(arg) for arg in args])
        indent = kwargs.get('indent', 0)
        text = ' ' * (indent + kwargs.get('first_line_indent', 0)) + text
        text = text.replace('\n', '\n' + ' ' * indent)
        text = self._colored(text, kwargs.get('color', None))
        return text

    def _colored(self, s, c):
        if c and not self._file and sys.stdout.isatty():
            return color.colored(s, c)
        return s


def get_setrace_reqid_from_token(token):
    if not token or token.count('$') != 2:
        return token
    return token.split('$')[1]


def process_test_turn(client, experiments, test_case, turn_data, uuid, is_first_turn, should_trace, additional_options, session):
    tz = pytz.timezone('Europe/Moscow')

    result = DialogTurnResult()
    result.time_begin = to_unicode(datetime.now(tz))
    result.request = turn_data.request

    merged_experiments = turn_data.experiments.copy()
    if experiments is not None:
        for k, v in experiments.iteritems():
            merged_experiments[k] = v

    try:
        request_label = None
        if should_trace:
            request_label = 'vins_integration_tests: {0} {1}'.format(test_case.name, turn_data.request)
            rtlog.thread_local.begin_request(None)
            result.setrace_reqid = get_setrace_reqid_from_token(rtlog.thread_local.get_token())

        api_response = client.request(
            uuid,
            datetime.now(tz),
            turn_data.event,
            test_case.geo_info or LOCATION,
            experiments=merged_experiments,
            app_info=test_case.app_info,
            string_response=False,
            device_state=test_case.device_state,
            additional_options=additional_options,
            reset_session=is_first_turn,
            dialog_id=turn_data.dialog_id,
            lang=turn_data.lang,
            request_label=request_label,
            session=session
        )
        vins_response = VinsResponse()
        SpeechKitTestAPI.deserialize_result(api_response, vins_response)
        response_for_tests = convert_vins_response_for_tests(vins_response.to_dict())
        is_success, error = check_response(turn_data, response_for_tests)

    except Exception as e:
        result.time_end = to_unicode(datetime.now(tz))
        result.error = to_unicode(e)
        result.is_success = False
        return result
    finally:
        if should_trace:
            rtlog.thread_local.end_request()

    result.session = vins_response.sessions.get('', None)
    result.time_end = to_unicode(datetime.now(tz))
    result.response = response_for_tests.text,
    result.is_success = is_success
    result.error = error
    return result


def process_test_dialog(client, experiments, test_case, should_trace):
    experiments = copy.deepcopy(experiments)
    test_case = copy.deepcopy(test_case)

    test_user = None
    uuid = None
    additional_options = test_case.additional_options

    if test_case.user_tags:
        try:
            test_user = test_user_api.get_test_user(test_case.user_tags)
            uuid = test_user.uuid
            bass_options = {'client_ip': test_user.client_ip}
            bass_options.update(additional_options.get('bass_options', {}))
            additional_options['oauth_token'] = test_user.token
            additional_options['bass_options'] = bass_options
        except PersonalAssistantAPIError as e:
            return DialogAttemptResult(
                initialization_error=to_unicode(e),
                is_success=False
            )

    uuid = uuid or str(gen_uuid_for_tests())
    result_turns = []

    try:
        is_first_turn = True
        session = None
        for turn_data in test_case.dialog:
            result_turn = process_test_turn(client, experiments, test_case, turn_data, uuid, is_first_turn,
                                            should_trace, additional_options, session)
            is_first_turn = False
            session = result_turn.session
            result_turns.append(result_turn)
            if not result_turn.is_success:
                return DialogAttemptResult(turns=result_turns, is_success=False)
    finally:
        if test_user:
            test_user_api.free_test_user(test_user.login)

    return DialogAttemptResult(turns=result_turns, is_success=True)


def process_test_case(client_v1, client_v2, experiments_v1, experiments_v2, test_case, should_trace):
    attempts_v1 = []
    attempts_v2 = []

    count = 0
    success_v1 = 0
    success_v2 = 0
    fail_v1 = 0
    fail_v2 = 0

    while True:
        attempts_v1.append(process_test_dialog(client_v1, experiments_v1, test_case, should_trace))
        attempts_v2.append(process_test_dialog(client_v2, experiments_v2, test_case, should_trace))

        count += 1
        success_v1 += attempts_v1[-1].is_success
        success_v2 += attempts_v2[-1].is_success
        fail_v1 = count - success_v1
        fail_v2 = count - success_v2

        if count == 1 and fail_v1 == 0 and fail_v2 == 0:
            status = OK
            break
        if count == 2 and success_v1 == 0 and success_v2 == 0:
            status = IGNORE
            break
        if count == 9:
            if fail_v2 == 0 and fail_v1 >= 4:
                status = FIXED
                break
            if fail_v1 == 0 and fail_v2 >= 4:
                status = FAIL
                break
            if min(fail_v1, fail_v2, success_v1, success_v2) > 0 and abs(fail_v1 - fail_v2) <= 3:
                status = FLAKY
                break
        if count >= 20:
            if fail_v1 >= fail_v2 + 12:
                status = FIXED
            elif fail_v2 >= fail_v1 + 12:
                status = FAIL
            else:
                status = FLAKY
            break

    statistics = StatisticsOfAttempts(
        count=count,
        success_v1=success_v1,
        success_v2=success_v2,
        fail_v1=fail_v1,
        fail_v2=fail_v2,
    )

    return TestCaseResult(
        name=test_case.name,
        app_info=(test_case.app_info.to_dict() if test_case.app_info else {}),
        attempts_v1=attempts_v1,
        attempts_v2=attempts_v2,
        statistics=statistics,
        status=status,
    )


def parse_args():
    parser = argparse.ArgumentParser(add_help=True)

    parser.add_argument('--vins-url', metavar='URL', dest='vins_url', required=True,
                        help='A speechkit API url to test')
    parser.add_argument('--bass-url', metavar='URL', dest='bass_url', required=False,
                        help='Allows to overwrite BASS url')
    parser.add_argument('--req-wizard-url', metavar='URL', dest='req_wizard_url', required=False,
                        help='Allows to overwrite Request Wizard url')
    parser.add_argument('--experiments', dest='experiments', default='',
                        help='Allows to add extra experiment flags. Format: `key1:any value1;key2:any value2`')

    group1 = parser.add_argument_group('v1', 'Overridden parameters for first (stable) tested version')
    group1.add_argument('--vins-url-v1', dest='vins_url_v1', default=None)
    group1.add_argument('--bass-url-v1', dest='bass_url_v1', default=None)
    group1.add_argument('--req-wizard-url-v1', dest='req_wizard_url_v1', default=None)
    group1.add_argument('--experiments-v1', dest='experiments_v1', default=None)

    group2 = parser.add_argument_group('v2', 'Overridden parameters for second (rc) tested version')
    group2.add_argument('--vins-url-v2', dest='vins_url_v2', default=None)
    group2.add_argument('--bass-url-v2', dest='bass_url_v2', default=None)
    group2.add_argument('--req-wizard-url-v2', dest='req_wizard_url_v2', default=None)
    group2.add_argument('--experiments-v2', dest='experiments_v2', default=None)

    parser.add_argument('--json-dump', metavar='JSON_FILE', dest='json_dump', required=False,
                        help='Dumps test run results to a json file')
    parser.add_argument('--report', metavar='TXT_FILE', dest='report', required=False,
                        help='Dumps test run results to a text file')
    parser.add_argument('--summary', metavar='TXT_FILE', dest='summary', required=False,
                        help='Dumps test run summary to a text file')
    parser.add_argument('--voice', dest='voice', action='store_true',
                        help='Issue queries as voice instead of text')
    parser.add_argument('--trace', dest='should_trace', action='store_true', default=False,
                        help='Trace requests by SETrace')
    parser.add_argument('--fast', dest='is_fast_mode', action='store_true', default=False,
                        help='Fast mode: process only one platform for each test case')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true', default=False,
                        help='Be verbose')
    parser.add_argument("--placeholders", action="store", default='',
                        help="Allow replacing placeholders with provided values. "
                             "Format: `key1:any value1;key2:any value2`")
    parser.add_argument("--test-prefix", action="store", default='',
                        help="Allows to choose test cases which names start with specified prefix")

    args = parser.parse_args()

    def select(value, default):
        return default if value is None else value

    args.vins_url_v1 = select(args.vins_url_v1, args.vins_url)
    args.bass_url_v1 = select(args.bass_url_v1, args.bass_url)
    args.req_wizard_url_v1 = select(args.req_wizard_url_v1, args.req_wizard_url)
    args.experiments_v1 = select(args.experiments_v1, args.experiments)

    args.vins_url_v2 = select(args.vins_url_v2, args.vins_url)
    args.bass_url_v2 = select(args.bass_url_v2, args.bass_url)
    args.req_wizard_url_v2 = select(args.req_wizard_url_v2, args.req_wizard_url)
    args.experiments_v2 = select(args.experiments_v2, args.experiments)

    return args


def prepare_rtlog(should_trace):
    if not should_trace:
        return None
    fd, path = tempfile.mkstemp('_test_rtlog')
    os.close(fd)
    rtlog.activate(path, 'vins_integration_tests')
    return path


def main():
    args = parse_args()

    rtlog_path = prepare_rtlog(args.should_trace)
    stdout_dumper = ReportDumper(args.verbose)

    dumpers = DumperList()
    dumpers.append(stdout_dumper)
    if args.report:
        dumpers.append(ReportDumper(args.verbose, args.report))
    if args.summary:
        dumpers.append(ReportDumper(args.verbose, args.summary, summary_only=True))
    if args.json_dump:
        dumpers.append(FileJsonDumper(args.json_dump, flushing_period=timedelta(seconds=10)))

    placeholders = parse_placeholders(args.placeholders)

    experiments_v1 = parse_placeholders(args.experiments_v1)
    experiments_v2 = parse_placeholders(args.experiments_v2)

    if UNIPROXY_VINS_SESSIONS_FLAG not in experiments_v1 and UNIPROXY_VINS_SESSIONS_FLAG not in experiments_v2:
        stdout_dumper._print('Warning! Force adding experiment flag: %s' % UNIPROXY_VINS_SESSIONS_FLAG, color='yellow')
        experiments_v1[UNIPROXY_VINS_SESSIONS_FLAG] = 1
        experiments_v2[UNIPROXY_VINS_SESSIONS_FLAG] = 1

    client_v1 = SpeechKitTestAPI(
        vins_url=args.vins_url_v1,
        bass_url=args.bass_url_v1,
        req_wizard_url=args.req_wizard_url_v1,
        as_voice=args.voice
    )
    client_v2 = SpeechKitTestAPI(
        vins_url=args.vins_url_v2,
        bass_url=args.bass_url_v2,
        req_wizard_url=args.req_wizard_url_v2,
        as_voice=args.voice
    )

    has_broken = False

    for test_case in load_testcase('integration_data', placeholders, args.is_fast_mode):
        if not test_case.name.startswith(args.test_prefix):
            continue
        if test_case.status == 'skip' or test_case.status == 'xfail':
            continue
        result = process_test_case(client_v1, client_v2, experiments_v1, experiments_v2, test_case, args.should_trace)
        dumpers.dump(result)
        has_broken = has_broken or result.status == FAIL

    dumpers.finalize()
    if rtlog_path:
        os.remove(rtlog_path)

    return 1 if has_broken else 0


def do_main():
    sys.exit(main())


if __name__ == '__main__':
    do_main()
