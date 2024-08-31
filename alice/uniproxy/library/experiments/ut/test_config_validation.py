from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.events import Event
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format
import yatest.common
import json
import os


_experiments_dir = yatest.common.source_path("alice/uniproxy/experiments")


def load_events():
    res = []
    with open(yatest.common.source_path("alice/uniproxy/library/experiments/ut/events.jsons"), "r") as f:
        for l in f.readlines():
            if l:
                res.append(Event(json.loads(l)))
    return res


def load_json(p):
    with open(p, "r") as f:
        return json.load(f)


class FakeUniSystem:
    def __init__(self):
        self.event_patcher = None
        self.log_events = []
        self.uaas_flags = {}
        self.uaas_test_ids = []
        self.session_data = {
            "accept_invalid_auth": True,
            "auth_token": "27fbd96d-ec5b-4688-a54d-421d81aa8cd2",
            "device": "Neffos Neffos_C9A",
            "device_manufacturer": "Neffos",
            "device_model": "Neffos_C9A",
            "emotion": "neutral", "network_type": "MOBILE:LTE:CONNECTED",
            "platform_info": "android",
            "speechkitVersion": "4.3.0",
            "speed": "1",
            "uuid": "ccb35d05f7394e04bb8edd6c6f96272e",
            "vins": {
                "application": {
                    "app_id": "ru.yandex.yandexnavi",
                    "app_version": "4.20",
                    "device_id": "d16ca0a2ce53bcda963c8593daff5874",
                    "device_manufacturer": "Neffos",
                    "device_model": "Neffos_C9A",
                    "os_version": "8.1.0",
                    "platform": "android",
                    "uuid": "ccb35d05f7394e04bb8edd6c6f96272e"
                }
            },
            "voice": "shitova.us",
            "yandexuid": ""
        }
        self.exps_check = True

    def set_event_patcher(self, patcher):
        self.event_patcher = patcher

    def log_experiment(self, log_event):
        self.log_events.append(log_event)

    def patch_event(self, event):
        request = event.payload.get('request')
        if request:
            if 'experiments' in request:
                request['experiments'] = safe_experiments_vins_format(request['experiments'], None)
            if 'megamind_cookies' in request:
                event.payload['cookie'] = request['megamind_cookies']
        if self.event_patcher:
            self.event_patcher.patch(event, self.session_data, hide_exceptions=False)


def test_validate_cexperiment_config():
    experiments_list = load_json(os.path.join(_experiments_dir, "experiments_rtc_production.json"))
    macro_list = load_json(os.path.join(_experiments_dir, "vins_experiments.json"))

    us = FakeUniSystem()

    experiments = Experiments(experiments_list, macro_list, mutable_shares=True)
    assert experiments.try_use_experiment(us)

    # must be no exceptions here
    for event in load_events():
        us.patch_event(event)


def test_no_unsupported_fields():
    for exp_file in ("experiments_rtc_production.json", "experiments_ycloud.json"):
        for exp in load_json(os.path.join(_experiments_dir, exp_file)):
            assert set(exp.keys()) == set(["id", "share", "flags"])
