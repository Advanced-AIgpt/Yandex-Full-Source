import inspect
from dataclasses import dataclass

import alice.tests.library.mark as mark
import alice.tests.library.url as url
import pytest
import yatest.common
from alice.tests.library.mark import Mark
from alice.tests.library.uniclient import AliceSettings


@dataclass
class TestArgs(object):
    uniproxy_url: str
    vins_url: str
    experiments: dict
    debug_mode: str
    use_prod: bool
    flaky_runs: int


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
        use_prod=_get('yes-i-want-to-kill-the-production'),
        flaky_runs=int(_get('flaky-runs', 3)),
    )
    if args.uniproxy_url == url.Uniproxy.prod or (args.vins_url and args.vins_url.startswith(url.Vins.prod)):
        assert args.use_prod, ('You specified production Uniproxy or/and Vins (aka Megamind). '
                               'Please confirm with "--test-param yes-i-want-to-kill-the-production=true"')

    return args


test_args = None


def pytest_configure(config):
    for mark_name in Mark.names:
        config.addinivalue_line('markers', f'{mark_name}: ...')


def pytest_sessionstart():
    global test_args
    test_args = parse_args()


def pytest_collection_modifyitems(items):
    flaky_max_runs = test_args.flaky_runs
    if test_args.debug_mode:
        flaky_max_runs = 1

    for item in items:
        if 'flaky' not in item.keywords and 'xfail' not in item.keywords:
            item.add_marker(pytest.mark.flaky(max_runs=flaky_max_runs, min_passes=1))


def _is_repeated_on_hamster_request(request):
    return request.node._flaky_current_runs >= test_args.flaky_runs


def _setdefault_experiment(exps, key, default_value):
    real_key, _ = key.split('=')
    for k in exps.keys():
        if k.startswith(real_key):
            return
    exps[key] = default_value


@pytest.fixture(scope='function')
def alice(request, surface):
    settings = AliceSettings()
    settings.log_type = 'cerr' if test_args.debug_mode else 'null'

    settings.uniproxy_url = url.Uniproxy.hamster
    settings.experiments = mark.get_experiments(request)
    settings.experiments.setdefault('analytics_info', '1')
    settings.experiments.setdefault('only_100_percent_flags', '1')
    _setdefault_experiment(settings.experiments, 'yamusic_audiobranding_score=0', '1')
    settings.experiments.setdefault('mordovia_video_base_url', 'https://hamster.yandex.ru')

    if not _is_repeated_on_hamster_request(request):
        settings.uniproxy_url = test_args.uniproxy_url
        settings.vins_url = test_args.vins_url
        settings.experiments.update(test_args.experiments)

    settings.application = mark.get_application(request)
    settings.oauth_token = mark.get_oauth_token(request)
    settings.region = mark.get_region(request)
    settings.iot = mark.get_iot(request)
    settings.supported_features = mark.get_supported_features(request)
    settings.unsupported_features = mark.get_unsupported_features(request)

    if inspect.isfunction(surface):
        surface = surface()
    surface.keywords['device_state'] = dict(
        mark.get_device_state(request),
        **surface.keywords['device_state'],
    )
    return surface(settings)
