import uuid


class FakeRtLog:
    def __init__(self, token=None, session=None):
        self.request_context = {}
        self.token = token
        self.child = {}
        self.errors = []
        self.exceptions = []
        self.log = []
        self.session = session

    def error(self, s):
        self.errors.append(s)

    def exception(self, s):
        self.exceptions.append(s)

    def log_request_context(self, **kwargs):
        self.request_context.update(kwargs)

    def log_child_activation_started(self, s):
        rtlog_token = str(uuid.uuid4())
        self.child[rtlog_token] = {'name': s}
        return rtlog_token

    def log_child_activation_finished(self, rtlog_token, success):
        self.child[rtlog_token].update({'success': success})

    def __call__(self, msg, **kwargs):
        self.log.append({'msg': msg}.update(kwargs))


def fake_begin_request(token=None, session=None):
    return FakeRtLog(token, session)
