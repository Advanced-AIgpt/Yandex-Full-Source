import threading
from rtlog.client import is_active, begin_request as base_begin_request


thread_data = threading.local()


def _get_request_logger():
    return getattr(thread_data, 'request_logger', None)


def begin_request(token):
    if is_active():
        thread_data.request_logger = base_begin_request(token)


def end_request():
    request_logger = _get_request_logger()
    if request_logger:
        del thread_data.request_logger
        request_logger.end_request()


def log(_message, **kwargs):
    request_logger = _get_request_logger()
    if request_logger:
        request_logger(_message, **kwargs)


def error(_message, **kwargs):
    request_logger = _get_request_logger()
    if request_logger:
        request_logger.error(_message, **kwargs)


def log_child_activation_started(description, new_req_id=False):
    request_logger = _get_request_logger()
    return request_logger and request_logger.log_child_activation_started(description, new_req_id)


def log_child_activation_finished(token, ok):
    request_logger = _get_request_logger()
    if request_logger:
        request_logger.log_child_activation_finished(token, ok)


def log_request_context(**kwargs):
    request_logger = _get_request_logger()
    if request_logger:
        request_logger.log_request_context(**kwargs)


def get_token():
    request_logger = _get_request_logger()
    return request_logger and request_logger.get_token()
