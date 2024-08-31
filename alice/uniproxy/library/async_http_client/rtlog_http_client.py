from tornado import gen


@gen.coroutine
def fetch_http(http_client, *args, **kwargs):
    rt_log = kwargs.pop('rt_log', None)
    rt_log_label = kwargs.pop('rt_log_label', None)
    rtlog_token = rt_log.log_child_activation_started(rt_log_label) if rt_log and rt_log_label else ''
    result = None
    try:
        result = yield http_client.fetch(*args, **kwargs)
    finally:
        if rtlog_token:
            rt_log.log_child_activation_finished(rtlog_token, result and result.code == 200)
    raise gen.Return(result)
