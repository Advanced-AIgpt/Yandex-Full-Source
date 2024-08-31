from functools import partial
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.global_counter import GlobalCounter

from .httpstream import HttpStream


def check_key(key, client_ip, on_ok, on_fail, rt_log=None, service_name=None):
    rtlog_token = None

    def check_local_key(key):
        return key in config["apikeys"]["whitelist"]

    def on_result(key, response):
        if rt_log:
            rt_log.log_child_activation_finished(rtlog_token, True)
        GlobalCounter.increment_error_code("apikeys", response.code)
        on_ok(key)

    def on_error(response):
        if rt_log:
            rt_log.log_child_activation_finished(rtlog_token, False)
        GlobalCounter.increment_error_code("apikeys", response.code)
        on_fail()

    def check_remote_key(key):
        nonlocal rtlog_token
        if rt_log:
            rtlog_token = rt_log.log_child_activation_started('check_key')
        service_token = config["apikeys"]["mobile_token"]
        if service_name and service_name == 'jsapi':
            # legacy (websocket /ttssocket.ws & /asrsocket.ws) api has it own service token
            service_token = config["apikeys"]["js_token"]
        url = "%s/check_key?service_token=%s&key=%s&user_ip=%s&ip_v=%s" % (
            config["apikeys"]["url"],
            service_token,
            key,
            client_ip,
            '6' if client_ip.count(':') else '4'
        )
        HttpStream(
            url,
            on_result=partial(on_result, key),
            on_error=on_error,
        )

    if key:
        if check_local_key(key):
            on_ok(key)
        else:
            check_remote_key(key)
    else:
        on_fail()
