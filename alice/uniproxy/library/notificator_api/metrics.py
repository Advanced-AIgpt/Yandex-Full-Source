import time

from alice.uniproxy.library.async_http_client.http_request import HTTPError


class MetricsBackend:
    __instance__ = None

    def __init__(self):
        pass

    def rate(self, name, count=1):
        pass

    def hgram(self, name, duration):
        pass

    @staticmethod
    def instance():
        if MetricsBackend.__instance__ is None:
            MetricsBackend.__instance__ = MetricsBackend()
        return MetricsBackend.__instance__


class NullScopedMetric:
    def __init__(self, label, name, backend):
        self.label = label
        self.name = name
        self.backend = backend or MetricsBackend.instance()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, err, traceback):
        pass


class ScopedMetric:
    def __init__(self, label, name, backend):
        self.label = label
        self.name = name
        self.backend = backend or MetricsBackend.instance()

    def make_success_counter(self):
        return '%s_OK_SUMM' % (self.name, )

    def make_error_counter(self):
        return '%s_ERR_SUMM' % (self.name, )

    def make_other_error_counter(self):
        return '%s_OTHER_ERR_SUMM' % (self.name, )

    def make_timeout_counter(self):
        return '%s_TIMEOUT_SUMM' % (self.name, )

    def make_time_counter(self):
        return ('%s_time' % (self.name, )).lower()

    def __enter__(self):
        self._started_at = time.time()
        return self

    def __exit__(self, exc_type, err, traceback):
        self.backend.hgram(self.make_time_counter(), time.time() - self._started_at)
        if exc_type is None:
            self.backend.rate(self.make_success_counter())
        elif exc_type is HTTPError:
            if err.code in (HTTPError.CODE_CONNECT_TIMEOUT, HTTPError.CODE_REQUEST_TIMEOUT):
                self.backend.rate(self.make_timeout_counter())
            else:
                self.backend.rate(self.make_error_counter())
        else:
            self.backend.rate(self.make_other_error_counter())


NOTIFICATION_METRICS_MAPPING = {
    'notificator_send_push': 'MATRIX_SEND_PUSH',
    'notificator_send_sup_push': 'MATRIX_SEND_SUP_PUSH',
    'notificator_send_sup_card': 'MATRIX_SEND_SUP_CARD',
    'notificator_delete_pushes': 'MATRIX_DELETE_PUSHES',
    'notificator_manage_subscription': 'MATRIX_MANAGE_SUBSCRIPTION',
    'notificator_get_notification_state': 'MATRIX_GET_STATE',
    'notificator_notification_change_status': 'MATRIX_CHANGE_STATUS',
    'ack_directive': 'MATRIX_ACK_DIRECTIVE',
    'notificator_on_connect': 'MATRIX_REGISTER',
    'notificator_push_typed_semantic_frame': 'MATRIX_PUSH_TYPED_SEMANTIC_FRAME',
    'matrix_add_schedule_action': 'MATRIX_ADD_SCHEDULE_ACTION',
}


def metrics_for(label, backend):
    name = NOTIFICATION_METRICS_MAPPING.get(label)
    if name is None:
        return NullScopedMetric(label, name, backend)
    else:
        return ScopedMetric(label, name, backend)
