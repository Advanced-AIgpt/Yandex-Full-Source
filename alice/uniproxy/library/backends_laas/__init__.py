import json
from alice.uniproxy.library.backends_common.httpstream import HttpStream
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter import GlobalCounter


def _mac_for_laas(macaddr):
    return macaddr.replace(':', '')


def _wifinets_to_arg(wifinets):
    try:
        arg = '&wifinetworks='
        args = []
        for net in wifinets:
            if len(net) != 2:
                continue

            mac = net.get('mac')
            strength = net.get('signal_strength')

            if not mac or not strength:
                continue

            mac = _mac_for_laas(mac)
            args.append(
                '%s:%d' % (mac, strength, )
            )

        if len(args) >= 1:
            return (True, '%s%s' % (arg, ','.join(args), ))
    except Exception:
        return (False, '')

    return (False, '')


def get_coords(client_ip, on_ok, rt_log, uuid=None, puid=None, wifinets=None, yandexuid=None, is_quasar=False, on_err=None):
    rtlog_token = None

    def on_result(response):
        rt_log.log_child_activation_finished(rtlog_token, True)
        GlobalCounter.increment_error_code("laas", response.code)
        try:
            res = json.loads(response.body.decode("utf-8"))
        except Exception as exc:
            Logger.get('.laas').exception(exc, rt_log=rt_log)
        else:
            on_ok(res)

    def on_error(response):
        rt_log.log_child_activation_finished(rtlog_token, False)
        if on_err:
            on_err(response)
        GlobalCounter.increment_error_code("laas", response.code)
        Logger.get('.laas').warning("Bad laas response:,", response.error, response.body, rt_log=rt_log)

    if config.get("laas", {}).get("url") is None:
        return

    query = '%s/region?real-ip=%s' % (config['laas']['url'], client_ip)
    headers = {}

    if is_quasar:
        query += '&service=quasar'

    if uuid is not None:
        query += '&uuid=%s' % (uuid, )

    if puid is not None:
        query += '&puid=%s' % (puid, )

    if wifinets:
        ok, arg = _wifinets_to_arg(wifinets)
        if ok:
            query += arg

    if yandexuid is not None:
        headers['Cookie'] = 'yandexuid=%s' % (yandexuid, )

    rtlog_token = rt_log.log_child_activation_started('laas')
    HttpStream(
        query,
        on_result=on_result,
        on_error=on_error,
        request_timeout=config["laas"].get("timeout", 0.2),
        headers=headers
    )
