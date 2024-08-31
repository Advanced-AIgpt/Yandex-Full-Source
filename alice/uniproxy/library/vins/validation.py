from urllib.parse import urlsplit
from alice.uniproxy.library.events import EventException


def _validate_vins_hostname(hostname):
    if hostname is None:
        raise EventException("forbidden")
    for h in (".yandex.ru", ".yandex-team.ru", ".yandex.net"):
        if hostname.endswith(h):
            return
    raise EventException("forbidden")


def _validate_vins_path(path):
    path = path.strip("/")
    if path == "speechkit":
        return
    if path.startswith("speechkit/app") and path.count("/") == 2:  # /speechkit/app/*/
        return
    raise EventException("forbidden")


def _validate_vins_url(url):
    url = urlsplit(url)
    _validate_vins_hostname(url.hostname)
    _validate_vins_path(url.path)


def validate_vins_event(event, allow_invalid=False):
    try:
        vins_url = event.payload.get("vinsUrl")
        if vins_url is not None:
            _validate_vins_url(vins_url)
    except EventException as err:
        err.event_id = event.message_id
        raise
