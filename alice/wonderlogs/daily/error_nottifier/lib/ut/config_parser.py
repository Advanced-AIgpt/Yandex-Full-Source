import json
import google.protobuf.text_format

from alice.wonderlogs.daily.error_nottifier.lib.config_parser import generate_thresholds
from alice.wonderlogs.protos.error_threshold_config_pb2 import TErrorThresholdConfig
from alice.wonderlogs.protos.megamind_prepared_pb2 import TMegamindPrepared
from alice.wonderlogs.protos.uniproxy_prepared_pb2 import TUniproxyPrepared

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
    }
]

MegamindPrepared [
    {
        Process: P_MEGAMIND_PREPARED_REDUCER
        Reason: R_INVALID_VALUE
        Thresholds {
            Warn {
                Share: 0.000001
            }
            Crit {
                Share: 0.00001
            }
        }
    }
]
'''

UNIPROXY_CONFIG_EXPECTED = '''
{
    "P_UNIPROXY_EVENT_DIRECTIVE_MAPPER":{
        "R_INVALID_JSON":{
            "Warn":{
                "Share":0.008
            },
            "Crit":{
                "Share":0.01
            }
        },
        "R_INVALID_VALUE":{
            "Warn":{
                "Share":0.05
            },
            "Crit":{
                "Share":0.1
            }
        }
    }
}
'''

MEGAMIND_PREPARED_EPECTED = '''
{
    "P_MEGAMIND_PREPARED_REDUCER":{
        "R_INVALID_VALUE":{
            "Warn":{
                "Share":1e-06
            },
            "Crit":{
                "Share":1e-05
            }
        }
    }
}
'''


def test_generate_thresholds():
    error_config = google.protobuf.text_format.Parse(CONFIG, TErrorThresholdConfig())

    thresholds = generate_thresholds(TUniproxyPrepared.TError, error_config.UniproxyPrepared)
    assert json.loads(UNIPROXY_CONFIG_EXPECTED) == thresholds

    thresholds = generate_thresholds(TMegamindPrepared.TError, error_config.MegamindPrepared)
    assert json.loads(MEGAMIND_PREPARED_EPECTED) == thresholds
