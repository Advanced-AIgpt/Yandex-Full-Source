from alice.megamind.api.utils.python import make_directive_with_typed_semantic_frame
from alice.megamind.protos.speechkit.directives_pb2 import TDirective
from alice.megamind.protos.common.frame_pb2 import TSemanticFrameRequestData
from google.protobuf import json_format

ETALON_SEARCH_SEMANTIC_FRAME = '''
{
    "payload": {
        "typed_semantic_frame": {
            "search_semantic_frame": {
                "query": {
                    "string_value": "utterance"
                }
            }
        },
        "analytics": {
            "product_scenario": "product",
            "origin": "Scenario",
            "purpose": "testing"
        },
        "origin": {
            "device_id": "another-device-id",
            "uuid": "another-uuid"
        }
    },
    "name": "@@mm_semantic_frame",
    "type": "server_action"
}
'''


def test_make_directive_with_typed_semantic_frame():
    SEARCH_SEMANTIC_FRAME = '''
    {
        "typed_semantic_frame": {
            "search_semantic_frame": {
                "query": {
                    "string_value": "utterance"
                }
            }
        },
        "analytics": {
            "product_scenario": "product",
            "origin": "Scenario",
            "purpose": "testing"
        },
        "origin": {
            "device_id": "another-device-id",
            "uuid": "another-uuid"
        }
    }
    '''

    req = TSemanticFrameRequestData()
    json_format.Parse(SEARCH_SEMANTIC_FRAME, req)

    serialized = make_directive_with_typed_semantic_frame(req.SerializeToString())
    directive = TDirective()
    directive.ParseFromString(serialized)

    etalon = TDirective()
    json_format.Parse(ETALON_SEARCH_SEMANTIC_FRAME, etalon)

    assert directive == etalon
