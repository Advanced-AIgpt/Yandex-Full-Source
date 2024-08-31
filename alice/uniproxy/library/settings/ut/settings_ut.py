from alice.uniproxy.library.settings import load_settings
from library.python import resource
import json


def test_validate_json():
    for name, data in resource.iteritems(prefix="/"):
        if not name.endswith(".json"):
            continue
        err = None
        try:
            json.loads(data)
        except Exception as e:
            err = e
        assert err is None, "Resource '{}' contains invalid JSON: {}".format(name, err)


def test_rtc_production_settings():
    config = load_settings('rtc_production')
    assert config
    assert config['port'] == 80
    assert config['asr']['yaldi_host'] == 'yaldi.alice.yandex.net'
    assert config['debug_logging'] is False
    assert config['subway']['debug'] is False
    assert 'test' not in config['notificator']['uniproxy']['url']
