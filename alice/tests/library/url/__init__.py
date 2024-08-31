import os
from dataclasses import dataclass
from urllib.parse import parse_qs, urlsplit, urlunsplit

import alice.tests.library.scenario as scenario


@dataclass
class _Url(object):
    prod: str
    hamster: str


Uniproxy = _Url(
    prod='wss://uniproxy.alice.yandex.net/uni.ws',
    hamster='wss://beta.uniproxy.alice.yandex.net/alice-uniproxy-hamster/uni.ws',
)


Vins = _Url(
    prod='http://vins.alice.yandex.net/speechkit/app/pa',
    hamster='http://vins.hamster.alice.yandex.net/speechkit/app/pa',
)


@dataclass
class _ApphostVersionUrl(_Url):
    prod: str = 'http://vins.alice.yandex.net'
    hamster: str = 'http://scenarios.hamster.alice.yandex.net'
    path: str = ''

    def __post_init__(self):
        self.path = os.path.join(self.path, 'utility')


@dataclass
class _ScenarioVersionUrl(_Url):
    srcrwr_name: str


_apphostVersionUrl = {
    'general_conversation': _ApphostVersionUrl(
        path='hollywood_general_conversation'
    ),
    'hollywood': _ApphostVersionUrl(
        path='hollywood_common'
    ),
}


_scenarioVersionUrls = {
    'dialogovo': _ScenarioVersionUrl(
        srcrwr_name=scenario.Dialogovo,
        prod='http://dialogovo.alice.yandex.net',
        hamster='http://paskills-common-testing.alice.yandex.net/dialogovo-hamster',
    ),
    'videobass': _ScenarioVersionUrl(
        srcrwr_name=scenario.Video,
        prod='http://video-scenario.alice.yandex.net',
        hamster='http://hamster.video-scenario.alice.yandex.net',
    ),
}


def _make_version_url(host, path='', query=''):
    if isinstance(host, str):
        host = urlsplit(host)
    path = os.path.join(path, 'version_json')
    return urlunsplit((host.scheme, host.netloc, path, query, ''))


def _parse_srcrwr(query):
    def _fix_url(name, url):
        if not url.startswith('http://'):
            url = f'http://{url}'
        return name, url

    srcrwr = parse_qs(query).get('srcrwr', [])
    return dict(_fix_url(*_.split(':', 1)) for _ in srcrwr)


def get_version_urls(args):
    vins_url = urlsplit(args.vins_url or Vins.hamster)

    version_urls = {}
    version_urls['megamind'] = _make_version_url(vins_url, query=vins_url.query)

    for key, url in _apphostVersionUrl.items():
        version_urls[key] = _make_version_url(vins_url, url.path, vins_url.query)

    vins_srcrwr = _parse_srcrwr(vins_url.query)
    for key, url in _scenarioVersionUrls.items():
        srcrwr_url = url.prod if args.use_prod else url.hamster
        srcrwr_url = vins_srcrwr.get(url.srcrwr_name, srcrwr_url)
        srcrwr_url = urlsplit(srcrwr_url)

        # fix dialogovo url
        srcrwr_path = srcrwr_url.path.split('/', 2)[1] if srcrwr_url.path else ''

        version_urls[key] = _make_version_url(srcrwr_url, srcrwr_path)

    return version_urls
