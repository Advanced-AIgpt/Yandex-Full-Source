from alice.uniproxy.library.processors.base_event_processor import EventProcessor


class FakeSystem:
    def __init__(self, session_data={}):
        self.session_data = session_data
        self.session_id = '123'
        self.icookie_for_uaas = None
        self.client_ip = 'ip'
        self.x_yandex_appinfo = ''
        self.device_id = 'id'
        self.test_ids = []
        self.use_balancing_hint = True
        self.use_spotter_glue = False
        self.use_laas = True
        self.use_datasync = True
        self.use_personal_cards = True
        self.srcrwr = {
            "FLAGS_JSON" : None
        }


class FakeEvent:
    def __init__(self, payload={}):
        self.payload = payload


class FakeLog:
    def debug(self, *args, **kwargs):
        pass

    def warning(self, *args, **kwargs):
        print(*args)

    def exception(self, *args, **kwargs):
        print(*args)

    def error(self, *args, **kwargs):
        print(*args)


def test_with_empty_icookie():
    system = FakeSystem({'uuid': '25228bce3cf7e8553b1954b38e3c5b7f'})
    proc = EventProcessor(system, None, None)
    proc._log = FakeLog()
    event = FakeEvent()
    proc._try_use_flags_json(event)
    assert system.icookie_for_uaas == '9958730751019942006'


def test_with_icookie():
    system = FakeSystem({'uuid': '25228bce3cf7e8553b1954b38e3c5b7f'})
    proc = EventProcessor(system, None, None)
    proc._log = FakeLog()

    event = FakeEvent()
    system.icookie_for_uaas = '12345'
    proc._try_use_flags_json(event)
    assert system.icookie_for_uaas == '12345'
