import getpass
import inspect
import json
import re
import operator
import logging
from collections.abc import Iterable
from dataclasses import dataclass
from datetime import datetime
from urllib.parse import urlparse, urlencode, urlunparse

import alice.tests.library.mark as mark
import alice.tests.library.solomon as solomon
import alice.tests.library.url as url
import alice.tests.library.ydb as ydb
import requests
import pytest
import yatest.common
from alice.tests.library.mark import Mark
from alice.tests.library.service import AppHost, Bass, Hollywood, Megamind
from alice.tests.library.surface import Muzpult, EvoError
from alice.tests.library.version import Version
from alice.tests.library.uniclient import AliceSettings
from alice.tests.library.uniclient.scraper_uniclient import ScraperClient
from yatest.common.network import PortManager

from . import alice_wrapper


@dataclass
class TestArgs(object):
    uniproxy_url: str
    vins_url: str
    experiments: dict
    debug_mode: bool
    regen_opus: bool
    use_prod: bool
    mark: str
    disable_version: bool
    flaky_runs: int
    repeat_failed_test: bool
    enable_stats: bool
    stats_timestamp: int
    sandbox_username: str
    sandbox_task: str
    sandbox_launch_type: str
    sandbox_branch_number: int
    sandbox_tag_number: int
    solomon_cluster: str
    solomon_service: str
    scraper_mode: bool
    from_evo_fifo: str
    to_evo_fifo: str
    scraper_input_table: str


def parse_args():
    def _get(name, default=None):
        return yatest.common.get_param(name, default)

    def _parse_exps(exps):
        def _parse_kv(kv):
            kv = kv.strip().split(':', 1)
            key = kv[0].rstrip()
            value = kv[1].lstrip() if len(kv) == 2 else '1'
            value = None if value == str(None) else value
            return key, value

        if not exps:
            return {}
        return dict(_parse_kv(_) for _ in exps.split(','))

    args = TestArgs(
        uniproxy_url=_get('uniproxy-url', url.Uniproxy.hamster),
        vins_url=_get('vins-url'),
        experiments=_parse_exps(_get('exps')),
        debug_mode=_get('debug'),
        regen_opus=_get('regen_opus'),
        use_prod=_get('yes-i-want-to-kill-the-production'),
        mark=_get('mark'),
        disable_version=_get('disable-version'),
        repeat_failed_test=_get('repeat-failed-test-on-hamster'),
        flaky_runs=int(_get('flaky-runs', 3)),
        enable_stats=_get('enable-stats'),
        stats_timestamp=_get('stats-timestamp'),
        sandbox_username=_get('sandbox-username'),
        sandbox_task=_get('sandbox-task'),
        sandbox_launch_type=_get('sandbox-launch-type'),
        sandbox_branch_number=_get('sandbox-branch-number'),
        sandbox_tag_number=_get('sandbox-tag-number'),
        solomon_cluster=_get('solomon-cluster'),
        solomon_service=_get('solomon-service'),
        scraper_mode=_get('scraper-mode'),
        from_evo_fifo=_get('from-evo-fifo'),
        to_evo_fifo=_get('to-evo-fifo'),
        scraper_input_table=_get('scraper-input-table'),
    )
    if args.uniproxy_url == url.Uniproxy.prod or (args.vins_url and args.vins_url.startswith(url.Vins.prod)):
        assert args.use_prod, ('You specified production Uniproxy or/and Vins (aka Megamind). '
                               'Please confirm with "--test-param yes-i-want-to-kill-the-production=true"')

    return args


def load_backend_version(args):
    def _get_version(url):
        try:
            response = requests.get(url)
            m = re.search(r'stable-(?P<version>\d+)', response.json()['branch'])
            return int(m['version'])
        except:
            return None

    urls = url.get_version_urls(args)
    return {name: _get_version(url) for name, url in urls.items()}


class ReleaseBugError(Exception):
    pass


test_args = None
backend_version = None


def is_flaky(err, *args):
    return not issubclass(err[0], EvoError)


def pytest_configure(config):
    for mark_name in Mark.names:
        config.addinivalue_line('markers', f'{mark_name}: ...')


def pytest_collection_finish(session):
    if test_args.scraper_input_table:
        with open(test_args.scraper_input_table, 'w') as scraper_input_table:
            for item in session.items:
                scraper_input_table.write(f'{item.nodeid}\n')
            scraper_input_table.flush()


def pytest_sessionstart():
    global test_args
    test_args = parse_args()
    global backend_version
    backend_version = Version(load_backend_version(test_args))
    logging.info(backend_version)

    if test_args.scraper_mode:
        ScraperClient.open_files(test_args.to_evo_fifo, test_args.from_evo_fifo)


def pytest_collection_modifyitems(items):
    flaky_max_runs = test_args.flaky_runs
    if test_args.debug_mode:
        flaky_max_runs = 1
    elif test_args.repeat_failed_test:
        flaky_max_runs *= 2

    for item in items:
        if 'flaky' not in item.keywords and 'xfail' not in item.keywords:
            item.add_marker(pytest.mark.flaky(max_runs=flaky_max_runs, min_passes=1, rerun_filter=is_flaky))


def pytest_runtest_setup(item):
    if test_args.mark and test_args.mark not in item.keywords:
        pytest.skip(f'run only with {test_args.mark} mark')

    version_marker = mark.get_version_marker(item)
    if not test_args.disable_version and version_marker:
        test_version = Version(version_marker.kwargs)
        comporator = version_marker.name.split('_')[-1]
        comporator = 'ge' if comporator == Mark.version else comporator
        if not getattr(operator, comporator)(backend_version, test_version):
            pytest.skip(f'Version must be {comporator} then required {test_version}')

    if test_args.scraper_mode:
        ScraperClient.send_start()


def pytest_generate_tests(metafunc):
    surface_marker = metafunc.definition.get_closest_marker(Mark.surface)
    if surface_marker:
        surfaces = set(surface_marker.args[0])
        exclude_surfaces = surface_marker.kwargs.get('exclude', [])
        if not isinstance(exclude_surfaces, Iterable):
            exclude_surfaces = [exclude_surfaces]
        metafunc.parametrize(Mark.surface, surfaces - set(exclude_surfaces))

    locale_marker = metafunc.definition.get_closest_marker(Mark.locale)
    if locale_marker:
        metafunc.parametrize(Mark.locale, locale_marker.args[0])


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    result = outcome.get_result()
    if result.when in ['setup', 'call']:
        item.test_result = result


def pytest_runtest_teardown(item):
    if item.test_result.skipped:
        return

    if item._flaky_current_runs >= test_args.flaky_runs and item.test_result.passed:
        raise ReleaseBugError('test url didn\'t work, but hamster url works')


def pytest_sessionfinish(session, exitstatus):
    if test_args.scraper_mode:
        ScraperClient.send_end_session()

    run_timestamp = int(test_args.stats_timestamp or datetime.now().timestamp())

    if test_args.solomon_cluster and test_args.solomon_service:
        solomon.send_sensors(test_args.solomon_cluster, test_args.solomon_service, run_timestamp, session.items)

    if test_args.debug_mode or not test_args.enable_stats:
        return

    username = getpass.getuser()
    is_sandbox = (username == 'sandbox')

    meta = dict(
        username=test_args.sandbox_username if is_sandbox else username,
        release=is_sandbox and test_args.sandbox_username == 'robot-testenv',
        sandbox=is_sandbox,
        sandbox_task=test_args.sandbox_task if is_sandbox else None,
        launch_type=test_args.sandbox_launch_type if is_sandbox else None,
        branch_number=test_args.sandbox_branch_number if is_sandbox else None,
        tag_number=test_args.sandbox_tag_number if is_sandbox else None,
    )

    def _to_ydb_row(item):
        return ydb.EvoStatsRow(
            timestamp=run_timestamp,
            name=item.nodeid,
            status=item.test_result.outcome,
            versions=backend_version,
            marks=[],
            extra_info=meta,
        )

    ydb.save([_to_ydb_row(item) for item in session.items])


def pytest_report_teststatus(report):
    if test_args.scraper_mode and report.when == 'call':
        ScraperClient.send_test_result(report)


def _is_repeated_on_hamster_request(request):
    return request.node._flaky_current_runs >= test_args.flaky_runs


def _setdefault_experiment(exps, key, default_value):
    real_key, _ = key.split('=')
    for k in exps.keys():
        if k.startswith(real_key):
            return
    exps[key] = default_value


def _is_local_service(service_flag):
    flags = yatest.common.context.flags
    return service_flag in flags or 'ALL' in flags


def _run_service(service, service_flag, port_manager, **kwargs):
    dummy_mode = not _is_local_service(service_flag)
    port = port_manager.get_port_range(None, service.port_count())
    return service(port, dummy=dummy_mode, **kwargs)


@pytest.fixture(scope='session')
def port_manager():
    with PortManager() as pm:
        yield pm


@pytest.fixture(scope='session')
def apphost(port_manager):
    with _run_service(AppHost, 'AP', port_manager) as service:
        service.wait_port()
        yield service


@pytest.fixture(scope='session')
def srcrwr_services(port_manager):
    with (
        _run_service(Bass, 'BASS', port_manager) as bass,
        _run_service(Megamind, 'MM', port_manager) as mm,
        _run_service(Hollywood, 'HW', port_manager, shard='all') as hw,
    ):
        yield [_ for _ in [bass, mm, hw] if _]


@pytest.fixture(scope='session')
def tanker_data():
    tanker_data_path = yatest.common.source_path('alice/tests/data/tanker_data.json')
    with open(tanker_data_path) as f:
        return json.load(f)


@pytest.fixture(scope='function')
def version(request):
    marker = mark.get_version_marker(request.node)
    if marker:
        return Version(marker.kwargs)


@pytest.fixture(scope='function')
def muzpult(request, alice):
    return Muzpult(alice)


@pytest.fixture(scope='function')
def reset_session(request, alice):
    alice.reset_session()


# this fixture is a fallback for tests without mark/parametrize of locale
@pytest.fixture(scope='function')
def locale():
    return None


@pytest.fixture(scope='function')
def alice(request, surface, apphost, srcrwr_services, tanker_data, locale):
    settings = AliceSettings()
    settings.scraper_mode = test_args.scraper_mode
    settings.log_type = 'cerr' if test_args.debug_mode else 'null'

    settings.uniproxy_url = url.Uniproxy.hamster
    settings.application = mark.get_application(request)
    settings.experiments = mark.get_experiments(request)
    settings.experiments.setdefault('analytics_info', '1')
    settings.experiments.setdefault('only_100_percent_flags', '1')
    settings.experiments.setdefault('mm_enable_session_reset', '1')
    _setdefault_experiment(settings.experiments, 'yamusic_audiobranding_score=0', '1')
    _setdefault_experiment(settings.experiments, 'station_promo_score=0', '1')
    settings.experiments.setdefault('mordovia_video_base_url', 'https://hamster.yandex.ru')

    if locale:
        alice_locale = locale()
        if inspect.isfunction(alice_locale):
            alice_locale = alice_locale()
        settings.application.setdefault('lang', alice_locale.name)
        for name, value in alice_locale.experiments.items():
            settings.experiments.setdefault(name, value)

    if not _is_repeated_on_hamster_request(request):
        settings.uniproxy_url = test_args.uniproxy_url
        settings.vins_url = test_args.vins_url
        settings.experiments.update(test_args.experiments)

    if apphost:
        settings.vins_url = apphost.url

    if srcrwr_services:
        settings.vins_url = settings.vins_url or url.Vins.hamster
        url_parts = list(urlparse(settings.vins_url))
        srcrwrs = [('srcrwr', _.srcrwr) for _ in srcrwr_services]
        url_parts[4] = urlencode(srcrwrs)
        settings.vins_url = urlunparse(url_parts)

    settings.oauth_token = mark.get_oauth_token(request)
    settings.region = mark.get_region(request)
    settings.iot = mark.get_iot(request)
    settings.supported_features = mark.get_supported_features(request)
    settings.unsupported_features = mark.get_unsupported_features(request)
    settings.permissions = mark.get_permissions(request)
    settings.predefined_contacts = mark.get_predefined_contacts(request)
    settings.environment_state = mark.get_environment_state(request)

    if inspect.isfunction(surface):
        surface = surface()
    surface.keywords['device_state'] = dict(
        mark.get_device_state(request),
        **surface.keywords['device_state'],
    )
    result = surface(settings)
    result = alice_wrapper.AliceWrapper(result, backend_version)
    if locale and alice_locale.use_tanker:
        test_group_id = request.node.nodeid.split('.')[0]
        result = alice_wrapper.TankerLocalizerAliceWrapper(result, tanker_data, alice_locale.name, test_group_id)
    if mark.is_voice_input(request):
        voice_marker = mark.get_input_marker(request)
        result = alice_wrapper.AliceVoiceWrapper(result, voice_marker, test_args.regen_opus)
    return result
