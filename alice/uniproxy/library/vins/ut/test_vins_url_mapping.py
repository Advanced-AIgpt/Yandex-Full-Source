import pytest

from tornado.httputil import HTTPHeaders

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils import Srcrwr, GraphOverrides
from alice.uniproxy.library.vins import VinsRequest


MOCK = type("VinsProcessorMock", (object,), {
    "_payload": {
        "application": {
            "app_id": "ru.yandex.searchplugin.beta",
            "app_version": "1.2.3",
            "os_version": "5.0",
            "platform": "android",
            "lang": "ru-RU",
            "timezone": "Europe/Moscow",
        },
        "request": {
            "additional_options": {
                "bass_options": {
                    "filtration_level": 1,
                    "user_agent": "monitoring"
                }
            },
            "event": {
                "type": "text_input"
            },
            "experiments": [
                ""
            ],
            "location": {
                "accuracy": 140,
                "lat": 52.26052093505859,
                "lon": 104.1884078979492,
                "recency": 192321
            },
            "reset_session": False,
            "voice_session": True
        },
    },
    "vins_url": VinsRequest._vins_url,
    "_build_vins_url": VinsRequest._build_vins_url,
    "_build_mapped_url": VinsRequest._build_mapped_url,
    "_parse_vins_url": VinsRequest._parse_vins_url,
    "_unparse_vins_url": VinsRequest._unparse_vins_url,
    "_parse_qs": VinsRequest._parse_qs,
    "_inject_graph_override": VinsRequest._inject_graph_override,
    "_make_application": VinsRequest._make_application,
    "_system": type(
        "System",
        (object,),
        {
            "session_data": {},
            "INFO": Logger.get().info,
            "DLOG": Logger.get().debug,
            "EXC": Logger.get().exception,
            "ERR": Logger.get().error,
            "WARN": Logger.get().warning,
            "srcrwr": Srcrwr(HTTPHeaders()),
            "graph_overrides": GraphOverrides([])
        }
    ),
    "INFO": Logger.get().info,
    "DLOG": Logger.get().debug,
    "EXC": Logger.get().exception,
    "ERR": Logger.get().error,
    "WARN": Logger.get().warning,
})()

VINS_BETA = "http://vins-int.voicetech.yandex.net/speechkit-prestable/app/pa/"
VINS = "http://vins.alice.yandex.net/speechkit/app/pa/"


def test_vins_url():
    assert (MOCK.vins_url() == VINS_BETA)

    MOCK._payload["application"]["app_id"] = "nope"

    assert (MOCK.vins_url() == VINS)

    del MOCK._payload["application"]["app_id"]

    assert (MOCK.vins_url() == VINS)

    del MOCK._payload["application"]

    assert (MOCK.vins_url() == VINS)

    MOCK._payload["vins"] = {"application": {"app_id": "ru.yandex.searchplugin.beta"}}

    assert (MOCK.vins_url() == VINS_BETA)

    MOCK._payload["application"] = {"app_id": "nope"}

    assert (MOCK.vins_url() == VINS)

    forced_url = "force-this-url"
    config.set_by_path("vins.url", forced_url)

    assert (MOCK.vins_url() == forced_url)

    config.set_by_path("vins.url", None)

    MOCK._system.srcrwr = Srcrwr(HTTPHeaders({Srcrwr.NAME: 'VINS=http://url/speechkit/app/pa/'}))
    assert MOCK._system.srcrwr['VINS'] == 'http://url/speechkit/app/pa/'
    assert MOCK.vins_url() == 'http://url/speechkit/app/pa/'


def test_vins_uaas_url_run():
    MOCK._payload["application"]["app_id"] = "ru.yandex.quasar"
    MOCK._system.srcrwr = Srcrwr(HTTPHeaders({}))

    MOCK._payload["vinsUrl"] = ''
    del MOCK._payload["vinsUrl"]

    assert (MOCK.vins_url() == "http://vins.alice.yandex.net/speechkit/app/pa/")
    assert (MOCK.vins_url("/speechkit/apply/") == "http://vins.alice.yandex.net/speechkit/apply/")

    MOCK._payload["vinsUrl"] = "http://vins.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://vins.alice.yandex.net/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply/") == "http://vins.alice.yandex.net/speechkit/apply/")
    del MOCK._payload["vinsUrl"]

    MOCK._payload["uaasVinsUrl"] = "http://apphost.vins.alice.yandex.net/?a=b"
    assert (MOCK.vins_url() == "http://apphost.vins.alice.yandex.net/speechkit/app/pa?a=b")
    assert (MOCK.vins_url("/speechkit/apply") == "http://apphost.vins.alice.yandex.net/speechkit/apply?a=b")

    MOCK._payload["uaasVinsUrl"] = "http://apphost.vins.alice.yandex.net/apphost"
    assert (MOCK.vins_url() == "http://apphost.vins.alice.yandex.net/apphost/speechkit/app/pa")
    assert (MOCK.vins_url("/speechkit/apply") == "http://apphost.vins.alice.yandex.net/apphost/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://vins.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://apphost.vins.alice.yandex.net/apphost/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://apphost.vins.alice.yandex.net/apphost/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://vins-int.voicetech.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://apphost.vins.alice.yandex.net/apphost/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://apphost.vins.alice.yandex.net/apphost/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://megamind-rc.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://megamind-rc.alice.yandex.net/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://megamind-rc.alice.yandex.net/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://foobar.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://foobar.alice.yandex.net/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://foobar.alice.yandex.net/speechkit/apply")


def test_vins_url_rwr_host():
    MOCK._payload["application"]["app_id"] = "ru.yandex.quasar"
    MOCK._system.srcrwr = Srcrwr(HTTPHeaders({Srcrwr.NAME: 'VINS_HOST=custom_host'}))

    MOCK._payload["vinsUrl"] = ''
    MOCK._payload["uaasVinsUrl"] = ''
    del MOCK._payload["vinsUrl"]
    del MOCK._payload["uaasVinsUrl"]

    assert (MOCK.vins_url() == "http://custom_host/speechkit/app/pa/")
    assert (MOCK.vins_url("/speechkit/apply/") == "http://custom_host/speechkit/apply/")

    MOCK._payload["vinsUrl"] = "http://vins.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://custom_host/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply/") == "http://custom_host/speechkit/apply/")
    del MOCK._payload["vinsUrl"]

    MOCK._payload["uaasVinsUrl"] = "http://apphost.vins.alice.yandex.net/?a=b"
    assert (MOCK.vins_url() == "http://apphost.vins.alice.yandex.net/speechkit/app/pa?a=b")
    assert (MOCK.vins_url("/speechkit/apply") == "http://apphost.vins.alice.yandex.net/speechkit/apply?a=b")

    MOCK._payload["uaasVinsUrl"] = "http://vins.alice.yandex.net/?a=b"
    assert (MOCK.vins_url() == "http://custom_host/speechkit/app/pa?a=b")
    assert (MOCK.vins_url("/speechkit/apply") == "http://custom_host/speechkit/apply?a=b")

    MOCK._payload["uaasVinsUrl"] = "http://apphost.vins.alice.yandex.net/apphost"
    assert (MOCK.vins_url() == "http://apphost.vins.alice.yandex.net/apphost/speechkit/app/pa")
    assert (MOCK.vins_url("/speechkit/apply") == "http://apphost.vins.alice.yandex.net/apphost/speechkit/apply")

    MOCK._payload["uaasVinsUrl"] = "http://vins.alice.yandex.net/apphost"
    assert (MOCK.vins_url() == "http://custom_host/apphost/speechkit/app/pa")
    assert (MOCK.vins_url("/speechkit/apply") == "http://custom_host/apphost/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://vins.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://custom_host/apphost/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://custom_host/apphost/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://vins-int.voicetech.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://custom_host/apphost/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://custom_host/apphost/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://megamind-rc.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://megamind-rc.alice.yandex.net/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://megamind-rc.alice.yandex.net/speechkit/apply")

    MOCK._payload["vinsUrl"] = "http://foobar.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://foobar.alice.yandex.net/speechkit/quasar")
    assert (MOCK.vins_url("/speechkit/apply") == "http://foobar.alice.yandex.net/speechkit/apply")


def test_vins_url_rwr_all():
    MOCK._payload["application"]["app_id"] = "ru.yandex.quasar"
    MOCK._system.srcrwr = Srcrwr(HTTPHeaders({}), ['VINS:http://vins.bob.yandex.net/speechkit/app/pa/', 'IoT:http://iot.y-t.ru/info'])

    MOCK._payload["vinsUrl"] = ''
    MOCK._payload["uaasVinsUrl"] = ''
    del MOCK._payload["vinsUrl"]
    del MOCK._payload["uaasVinsUrl"]

    assert (MOCK.vins_url() == "http://vins.bob.yandex.net/speechkit/app/pa/")
    assert (MOCK.vins_url("/speechkit/apply/") == "http://vins.bob.yandex.net/speechkit/apply/")

    MOCK._payload["vinsUrl"] = "http://vins.alice.yandex.net/speechkit/quasar"
    assert (MOCK.vins_url() == "http://vins.bob.yandex.net/speechkit/app/pa/")
    assert (MOCK.vins_url("/speechkit/apply/") == "http://vins.bob.yandex.net/speechkit/apply/")
    del MOCK._payload["vinsUrl"]


@pytest.mark.parametrize("cgi,payload,uaas,suffix,expected", [
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa",
        None,
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa"
    ),
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/",
        None,
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/",
    ),
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        None,
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        "http://alice.net/",
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        None,
        None,
        "http://alice.net/?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        "http://alice.net/",
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        None,
        '/quasar',
        "http://alice.net/quasar?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        "http://alice.net/",
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        "http://bob.net/apphost",
        None,
        "http://alice.net/?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        "http://alice.net/",
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        "http://bob.net/apphost",
        "/quasar",
        "http://alice.net/quasar?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        "http://bob.net/apphost",
        None,
        "http://bob.net/apphost/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        "http://bob.net/apphost?srcrwr=C:http://c.yandex.net/",
        None,
        "http://bob.net/apphost/speechkit/app/pa?srcrwr=C:http://c.yandex.net/&srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        "http://bob.net/apphost?srcrwr=A:http://aa.yandex.net/",
        None,
        "http://bob.net/apphost/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    ),
    (
        None,
        "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
        "http://bob.net/apphost",
        "/speechkit/quasar",
        "http://bob.net/apphost/speechkit/quasar?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/",
    )
])
def test_vins_url_builder(cgi, payload, uaas, suffix, expected):
    assert MOCK._build_vins_url(cgi, payload, uaas, suffix) == expected


def _test_vins_url_builder_perf():
    import time

    cgi = "http://alice.net/"
    payload = "http://vins.alice.yandex.net/speechkit/app/pa?srcrwr=A:http://a.yandex.net/&srcrwr=B:https://b.yandex.net/"
    uaas = "http://bob.net/apphost"
    suffix = "/quasar"

    count = 10000

    a = time.time()
    for i in range(0, count):
        MOCK._build_vins_url(cgi, payload, uaas, suffix)
    b = time.time()

    avg = (b - a) / count

    print('BUILD_VINS_URL AVG %.3fms' % (avg * 1000))

    assert avg < 0.0005


def test_vins_url_with_vins_rwr():
    vins_url = 'http://vins.hamster.alice.yandex.net/speechkit/app/pa/'
    vins_url += '?srcrwr=MEGAMIND_ALIAS:akhruslan.sas.yp-c.yandex.net:17893'
    vins_url += '&srcrwr=Vins:http://media-alice.sas.yp-c.yandex.net:9010/proto/app/pa/'
    vins_url += '&srcrwr=VINS:http://media-alice.sas.yp-c.yandex.net:9010/proto/app/pa/'

    MOCK._payload["application"]["app_id"] = "ru.yandex.quasar"
    MOCK._system.srcrwr = Srcrwr(HTTPHeaders({}), [])

    MOCK._payload["vinsUrl"] = ''
    MOCK._payload["uaasVinsUrl"] = ''
    del MOCK._payload["vinsUrl"]
    del MOCK._payload["uaasVinsUrl"]

    MOCK._payload["vinsUrl"] = vins_url

    url = MOCK.vins_url()

    assert 'vins.hamster.alice.yandex.net' in url
    assert 'srcrwr=MEGAMIND_ALIAS:akhruslan.sas.yp-c.yandex.net:17893' in url
    assert 'srcrwr=VINS:http://media-alice.sas.yp-c.yandex.net:9010/proto/app/pa/' in url
    assert 'srcrwr=Vins:http://media-alice.sas.yp-c.yandex.net:9010/proto/app/pa/' in url

    MOCK._system.srcrwr = Srcrwr(HTTPHeaders({}), ['VINS:http://vins.bob.yandex.net/speechkit/app/pa/', 'IoT:http://iot.y-t.ru/info'])
    url = MOCK.vins_url()

    assert 'vins.bob.yandex.net' in url
    assert 'srcrwr=MEGAMIND_ALIAS:akhruslan.sas.yp-c.yandex.net:17893' in url
    assert 'srcrwr=VINS:http://media-alice.sas.yp-c.yandex.net:9010/proto/app/pa/' in url
    assert 'srcrwr=Vins:http://media-alice.sas.yp-c.yandex.net:9010/proto/app/pa/' in url
