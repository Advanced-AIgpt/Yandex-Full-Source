import google.protobuf.text_format
import json
from alice.wonderlogs.daily.error_nottifier.lib.config_parser import generate_thresholds
from alice.wonderlogs.daily.error_nottifier.lib.reporter import report
from alice.wonderlogs.protos.error_threshold_config_pb2 import TErrorThresholdConfig
from alice.wonderlogs.protos.uniproxy_prepared_pb2 import TUniproxyPrepared

rows = '''
[
    {
        "process":"P_UNIPROXY_PREPARED_REDUCER",
        "reason":"R_DIFFERENT_VALUES",
        "count":1532082,
        "share":0.022618856467179226
    },
    {
        "process":"P_MEGAMIND_REQUEST_RESPONSE_REQUEST_STAT_REDUCER",
        "reason":"R_DIFFERENT_VALUES",
        "count":1515081,
        "share":0.022367862604710693
    },
    {
        "process":"P_UNIPROXY_EVENT_DIRECTIVE_MAPPER",
        "reason":"R_INVALID_VALUE",
        "count":3017032,
        "share":0.044541880764141006
    },
    {
        "process":"P_UNIPROXY_EVENT_DIRECTIVE_MAPPER",
        "reason":"R_INVALID_JSON",
        "count":311685,
        "share":0.004601554145256427
    }
]
'''

CONFIG = '''
UniproxyPrepared [
    {
        Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
        Reason: R_INVALID_JSON
        Thresholds {
            Warn {
                Share: 0.008
            }
            Crit {
                Share: 0.01
            }
        }
    },
    {
        Process: P_UNIPROXY_EVENT_DIRECTIVE_MAPPER
        Reason: R_INVALID_VALUE
        Thresholds {
            Warn {
                Share: 0.05
            }
            Crit {
                Share: 0.1
            }
        }
    },
    {
        Process: P_UNIPROXY_PREPARED_REDUCER
        Reason: R_DIFFERENT_VALUES
        Thresholds {
            Warn {
                Share: 0.03
            }
            Crit {
                Share: 0.05
            }
        }
    },
    {
        Process: P_MEGAMIND_REQUEST_RESPONSE_REQUEST_STAT_REDUCER
        Reason: R_DIFFERENT_VALUES
        Thresholds {
            Warn {
                Share: 0.03
            }
            Crit {
                Share: 0.05
            }
        }
    }
]
'''


def test_report():
    error_config = google.protobuf.text_format.Parse(CONFIG, TErrorThresholdConfig())

    thresholds = generate_thresholds(TUniproxyPrepared.TError, error_config.UniproxyPrepared)

    status, checks = report(json.loads(rows), thresholds)
    checks_expected = [('P_UNIPROXY_PREPARED_REDUCER', 'R_DIFFERENT_VALUES', 'OK'),
                       ('P_MEGAMIND_REQUEST_RESPONSE_REQUEST_STAT_REDUCER', 'R_DIFFERENT_VALUES', 'OK'),
                       ('P_UNIPROXY_EVENT_DIRECTIVE_MAPPER', 'R_INVALID_VALUE', 'OK'),
                       ('P_UNIPROXY_EVENT_DIRECTIVE_MAPPER', 'R_INVALID_JSON', 'OK')]

    assert 'OK' == status
    assert checks_expected == checks
