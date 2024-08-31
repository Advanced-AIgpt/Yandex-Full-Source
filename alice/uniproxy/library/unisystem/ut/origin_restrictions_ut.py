import alice.uniproxy.library.unisystem.uniwebsocket as uniwebsocket
from alice.uniproxy.library.unisystem.uniwebsocket import HUniWebSocket


def test_yandex_ru():
    assert(HUniWebSocket.check_origin(None, "https://messenger.yandex.ru"))
    assert(HUniWebSocket.check_origin(None, "https://www.yandex.ru"))
    assert(HUniWebSocket.check_origin(None, "https://www.some_service.yandex.ru"))
    assert(HUniWebSocket.check_origin(None, "https://yandex.ru"))
    assert(HUniWebSocket.check_origin(None, "https://chat-dev-rr-templates.hamster.yandex.ru"))
    assert(HUniWebSocket.check_origin(None, "https://http2.priemka.yandex.ru"))
    assert(HUniWebSocket.check_origin(None, "http://http2.priemka.yandex.ru"))


def test_local_yandex_ru():
    uniwebsocket.USE_LOCAL_DOMAIN_RESTRICTION_PATTERN = False
    assert(not HUniWebSocket.check_origin(None, "https://local.yandex.ru:3443"))

    uniwebsocket.USE_LOCAL_DOMAIN_RESTRICTION_PATTERN = True
    assert(HUniWebSocket.check_origin(None, "https://local.yandex.ru:3443"))


def check_domain(domain):
    assert(HUniWebSocket.check_origin(None, "https://messenger.yandex{}".format(domain)))
    assert(HUniWebSocket.check_origin(None, "https://www.yandex{}".format(domain)))
    assert(HUniWebSocket.check_origin(None, "https://www.some_service.yandex{}".format(domain)))
    assert(HUniWebSocket.check_origin(None, "https://yandex{}".format(domain)))


def test_national_domains():
    domains = [
        '.ru', '.by', '.ua', '.kz', '.com', '.com.tr', '.com.am',
        '.com.ge', '.az', '.co.il', '.kg', '.lv', '.lt', '.md',
        '.tj', '.tm', '.uz', '.ee', '.fr',
    ]

    for domain in domains:
        check_domain(domain)


def test_yandex_team_ru():
    assert(HUniWebSocket.check_origin(None, "https://messenger.yandex-team.ru"))
    assert(HUniWebSocket.check_origin(None, "https://voiceservices.tst.voicetech.yandex.ru"))


def test_bad_domains():
    assert(not HUniWebSocket.check_origin(None, "https://huyandex.ru"))
    assert(not HUniWebSocket.check_origin(None, "https://yandex-team.net"))
    assert(not HUniWebSocket.check_origin(None, "https://messenger.ru"))
    assert(not HUniWebSocket.check_origin(None, "https://yandex.net"))
    assert(not HUniWebSocket.check_origin(None, "https://www.yandex.net"))


def test_www_ya_ru():
    assert(HUniWebSocket.check_origin(None, "https://www.ya.ru"))
    assert(HUniWebSocket.check_origin(None, "https://ya.ru"))
    assert(not HUniWebSocket.check_origin(None, "https://http2.ya.ru"))
    assert(not HUniWebSocket.check_origin(None, "https://www.some_service.ya.ru"))


def check_no_ya_domain(domain):
    assert(not HUniWebSocket.check_origin(None, "https://www.ya{}".format(domain)))
    assert(not HUniWebSocket.check_origin(None, "https://ya{}".format(domain)))
    assert(not HUniWebSocket.check_origin(None, "https://http2.ya{}".format(domain)))
    assert(not HUniWebSocket.check_origin(None, "https://www.some_service.ya{}".format(domain)))


def test_no_ya_national_domains():
    domains = [
        '.by', '.ua', '.kz', '.com', '.com.tr', '.com.am',
        '.com.ge', '.az', '.co.il', '.kg', '.lv', '.lt', '.md',
        '.tj', '.tm', '.uz', '.ee', '.fr',
    ]

    for domain in domains:
        check_no_ya_domain(domain)
