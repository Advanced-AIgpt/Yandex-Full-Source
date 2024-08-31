import json
import logging
from base64 import b64encode

from google.protobuf import json_format

import alice.tests.library.mark as mark
from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo
from alice.memento.proto.api_pb2 import TRespGetAllObjects


logger = logging.getLogger(__name__)

# For now we use the same default for all requests
IOT_USER_INFO = '''
{
    "raw_user_info": "{\\"payload\\": {\\"devices\\": []}}",
    "devices": []
}
'''

MEMENTO = '''
{
    "UserConfigs": {}
}
'''

CONTACTS = '''
{
    "data": {
        "contacts": [],
        "phones": []
    },
    "status": "ok"
}
'''


Mark = mark.MarkObject(additional_marks=[
    'scenario',
    'additional_options',
    'iot_user_info',
    'memento',
    'contacts',
    'freeze_stubs',
    'evo',
    'scenario_state',
    'notification_state',
])


get_application = mark.get_application
get_closest_marker = mark.get_closest_marker
get_region = mark.get_region
get_oauth_token = mark.get_oauth_token
get_experiments = mark.get_experiments
get_device_state = mark.get_device_state
get_environment_state = mark.get_environment_state
get_supported_features = mark.get_supported_features
get_unsupported_features = mark.get_unsupported_features
is_voice_input = mark.is_voice_input


def get_scenario(request):
    marker = request.node.get_closest_marker(Mark.scenario)
    if not marker:
        raise Exception('Test does not have mandatory @pytest.mark.scenario(name=<...>, handle=<...>) declared')

    return marker.kwargs.get('name'), marker.kwargs.get('handle')


def get_iot_user_info(request):
    marker = request.node.get_closest_marker(Mark.iot_user_info)
    iot_user_info = IOT_USER_INFO
    if marker:
        assert len(marker.args) == 1
        iot_user_info = marker.args[0]

    iot_user_info_proto = TIoTUserInfo()
    json_format.Parse(iot_user_info, iot_user_info_proto)
    return b64encode(iot_user_info_proto.SerializeToString()).decode()


def get_memento(request):
    marker = request.node.get_closest_marker(Mark.memento)
    memento = MEMENTO
    if marker:
        assert len(marker.args) == 1
        memento = marker.args[0]
    if isinstance(memento, str):
        memento = json.loads(memento)

    memento_proto = TRespGetAllObjects()
    json_format.ParseDict(memento, memento_proto)
    return b64encode(memento_proto.SerializeToString()).decode()


def get_contacts(request):
    marker = request.node.get_closest_marker(Mark.contacts)
    contacts = CONTACTS
    if marker:
        assert len(marker.args) == 1
        contacts = marker.args[0]
    if isinstance(contacts, str):
        contacts = json.loads(contacts)

    return contacts


def get_notification_state(request):
    marker = request.node.get_closest_marker(Mark.notification_state)
    notification_state = None
    if marker:
        assert len(marker.args) == 1
        notification_state = marker.args[0]
    if isinstance(notification_state, str):
        notification_state = json.loads(notification_state)
    return notification_state


def get_additional_options(request):
    additional_options = {}
    for marker in request.node.iter_markers(Mark.additional_options):
        for pos_arg in marker.args:
            additional_options.update(pos_arg)
        additional_options.update(marker.kwargs)

    return additional_options


def get_freeze_stubs(request):
    marker = request.node.get_closest_marker(Mark.freeze_stubs)
    return marker.kwargs if marker else {}


def get_scenario_state(request):
    marker = request.node.get_closest_marker(Mark.scenario_state)
    if marker:
        assert len(marker.args) == 1
        scenario_state = marker.args[0]
    else:
        scenario_state = None
    return scenario_state
