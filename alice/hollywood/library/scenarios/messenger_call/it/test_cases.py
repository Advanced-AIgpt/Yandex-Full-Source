from alice.hollywood.library.python.testing.integration.conftest import create_hollywood_fixture
from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import make_generator_params, \
    make_runner_params
from alice.library.python.testing.megamind_request.input_dialog import voice

from alice.megamind.protos.common.data_source_type_pb2 import IOT_USER_INFO
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest

from google.protobuf import text_format

TESTS_DATA_PATH = 'alice/hollywood/library/scenarios/messenger_call/it/data/'

SCENARIO_NAME = 'MessengerCall'
SCENARIO_HANDLE = 'messenger_call'

hollywood = create_hollywood_fixture([SCENARIO_HANDLE])

# For available presets see alice/acceptance/modules/request_generator/lib/app_presets.py
DEFAULT_APP_PRESETS = ['quasar']

EMERGENCY_APP_PRESETS = ['quasar', 'search_app_prod', 'elariwatch', 'auto']

DEVICE_CALL_APP_PRESETS = ['search_app_prod']

DEVICE_SHORTCUT_APP_PRESETS = ['quasar', 'search_app_prod', 'auto']

DEFAULT_EXPERIMENTS = [
    'enable_outgoing_device_calls',
    'bg_fresh_granet_form=alice.messenger_call.get_caller_name',
    'bg_fresh_granet_form=alice.messenger_call.stop_current_call',
    'bg_fresh_granet_form=alice.messenger_call.stop_incoming_call',
    'bg_fresh_granet_form=alice.messenger_call.accept_incoming_call',
    'bg_fresh_granet_form=alice.messenger_call.call_to.ifexp.bg_enable_call_to_form_v2',
    'bg_fresh_granet_form=alice.messenger_call.call_target.ifexp.bg_enable_call_to_form',
    'bg_fresh_granet_form=alice.messenger_call.can_you_call',
    'bg_fresh_granet_form=alice.messenger_call.device_call_shortcut',
    'bg_fresh_granet_form=alice.messenger_call.device_call_forced_shortcut',
    'bg_enable_call_to_form_v2',
    'bg_enable_call_to_form',
]

DEFAULT_SUPPORTED_FEATURES = ['incoming_messenger_calls']


def _make_incoming_call_device_state(mics_muted=False):
    return {
        'messenger_call': {
            'incoming': {
                'call_id': 'some_call_id',
                'recipient_id': 'some_recipient_id',
                'caller_name': 'some caller name',
            }
        },
        'mics_muted': mics_muted
    }


def _make_current_call_device_state():
    return {
        'messenger_call': {
            'current': {
                'call_id': 'some_call_id',
                'recipient_id': 'some_recipient_id',
                'caller_name': 'some caller name',
            }
        }
    }


class _Device(object):

    def __init__(self, name, platform, device_id, room_id=None):
        self.name = name
        self.platform = platform
        self.device_id = device_id
        self.room_id = room_id


class _Room(object):

    def __init__(self, name, room_id):
        self.name = name
        self.room_id = room_id


class _DeviceSlot(object):

    def __init__(self, smart_home_id):
        self.smart_home_id = smart_home_id

    def dump(self, slot):
        slot.Name = "device"
        slot.Type = "device.iot.device"
        slot.Value = "device--" + self.smart_home_id
        slot.AcceptedTypes.append("device.iot.device")
        slot.IsFilled = True


class _RoomSlot(object):

    def __init__(self, smart_home_id):
        self.smart_home_id = smart_home_id

    def dump(self, slot):
        slot.Name = "room"
        slot.Type = "device.iot.room"
        slot.Value = self.smart_home_id
        slot.AcceptedTypes.append("device.iot.room")
        slot.IsFilled = True


def _inject_iot_user_info(devices, rooms, slots=None):

    def patcher(run_request):
        run_request_proto = TScenarioRunRequest()
        text_format.Merge(run_request, run_request_proto)

        info = run_request_proto.DataSources[IOT_USER_INFO].IoTUserInfo

        for device in devices:
            proto_device = info.Devices.add()

            proto_device.Id = 'smart_home_' + device.device_id
            proto_device.Name = device.name
            proto_device.AnalyticsType = 'type'
            if device.room_id:
                proto_device.RoomId = device.room_id
            proto_device.QuasarInfo.DeviceId = device.device_id
            proto_device.QuasarInfo.Platform = device.platform

        for room in rooms:
            proto_room = info.Rooms.add()

            proto_room.Id = room.room_id
            proto_room.Name = room.name

        # TODO(akastornov): avoid this hack
        if slots is not None:
            del run_request_proto.Input.SemanticFrames[:]
            sf = run_request_proto.Input.SemanticFrames.add()
            sf.Name = "alice.messenger_call.call_to"
            for s in slots:
                s.dump(sf.Slots.add())

        return text_format.MessageToString(run_request_proto, as_utf8=True)

    return patcher


DEVICE_CALL_TESTS_DATA = {
    'call_to_device': {
        'input_dialog': [
            voice(
                'позвони в слона',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1')],
                    [],
                    [_DeviceSlot('smart_home_id1')]
                )
            )
        ]
    },
    'call_to_room': {
        'input_dialog': [
            voice(
                'позвони в кабинет',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1', 'room1')],
                    [_Room('кабинет', 'room1')],
                    [_RoomSlot('room1')]
                )
            )
        ]
    },
    'call_to_platform_yandexmini': {
        'input_dialog': [
            voice(
                'позвони в миник',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1')],
                    []
                )
            )
        ]
    },
    'call_to_platform_yandexstation': {
        'input_dialog': [
            voice(
                'позвони в станцию',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexstation', 'id1')],
                    []
                )
            )
        ]
    },
    'call_to_platform_yandexstation_2': {
        'input_dialog': [
            voice(
                'позвони в станцию макс',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexstation_2', 'id1')],
                    []
                )
            )
        ]
    },
    'call_to_device_two_devices_target_supports_calls': {
        'input_dialog': [
            voice(
                'позвони в слона',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1'), _Device('мышь', 'yandexmini', 'id2')],
                    [],
                    [_DeviceSlot('smart_home_id1')]
                )
            )
        ]
    },
    'call_to_device_two_devices_target_doesnt_support_calls': {
        'input_dialog': [
            voice(
                'позвони в слона',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('мышь', 'yandexmini', 'id2')],
                    [],
                    [_DeviceSlot('smart_home_id1')]
                )
            )
        ]
    },
    'call_to_room_two_devices_one_supports_calls': {
        'input_dialog': [
            voice(
                'позвони в кабинет',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1', 'room1'), _Device('мышь', 'dexpp', 'id2', 'room1')],
                    [_Room('кабинет', 'room1')],
                    [_RoomSlot('room1')]
                )
            )
        ]
    },
    'call_to_room_two_devices_two_support_calls': {
        'input_dialog': [
            voice(
                'позвони в кабинет',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1', 'room1'), _Device('мышь', 'yandexmini', 'id2', 'room1')],
                    [_Room('кабинет', 'room1')],
                    [_RoomSlot('room1')]
                )
            )
        ]
    },
    'call_to_room_two_devices_noone_supports_calls': {
        'input_dialog': [
            voice(
                'позвони в кабинет',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1', 'room1'), _Device('мышь', 'dexpp', 'id2', 'room1')],
                    [_Room('кабинет', 'room1')],
                    [_RoomSlot('room1')]
                )
            )
        ]
    },
    'call_to_room_two_devices_noone_supports_calls_have_device_supporting_calls': {
        'input_dialog': [
            voice(
                'позвони в кабинет',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1', 'room1'), _Device('мышь', 'dexpp', 'id2', 'room1'), _Device('соль', 'yandexmini', 'id3')],
                    [_Room('кабинет', 'room1')],
                    [_RoomSlot('room1')]
                )
            )
        ]
    },
    'call_to_any_device_no_devices_support_calls': {
        'input_dialog': [
            voice(
                'позвони в колонку',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('мышь', 'dexpp', 'id2')],
                    []
                )
            )
        ]
    },
    'call_to_any_device_one_device_supports_calls': {
        'input_dialog': [
            voice(
                'позвони в колонку',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('мышь', 'yandexmini', 'id2')],
                    []
                )
            )
        ]
    },
    'call_to_device_two_devices_same_name_both_support_calls': {
        'input_dialog': [
            voice(
                'позвони в слона',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1'), _Device('слон', 'yandexmini', 'id2')],
                    [],
                    [_DeviceSlot('smart_home_id1')]
                )
            )
        ]
    },
    'call_to_device_two_devices_same_name_one_supports_calls': {
        'input_dialog': [
            voice(
                'позвони в слона',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('слон', 'yandexmini', 'id2')],
                    [],
                    [_DeviceSlot('smart_home_id1')]
                )
            )
        ]
    },
    'call_to_device_two_devices_same_name_noone_supports_calls': {
        'input_dialog': [
            voice(
                'позвони в слона',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('Слон', 'dexpp', 'id2')],
                    [],
                    [_DeviceSlot('smart_home_id1')]
                )
            )
        ]
    },
    'call_to_room_two_rooms_same_name': {
        'input_dialog': [
            voice(
                'позвони в кабинет',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'yandexmini', 'id1', 'room1'), _Device('мышь', 'yandexmini', 'id2', 'room2')],
                    [_Room('кабинет', 'room1'), _Room('кабинет', 'room2')],
                    [_RoomSlot('room1')]
                )
            )
        ]
    },
}


EMERGENCY_CALL_TESTS_DATA = {
    'call_to_police': {
        'input_dialog': [
            voice('позвони в полицию')
        ],
    },
    'call_to_ambulance': {
        'input_dialog': [
            voice('позвони в скорую')
        ],
    },
    'call_to_fire_repartment': {
        'input_dialog': [
            voice('позвони в пожарную')
        ],
    },
    'call_to_emergency': {
        'input_dialog': [
            voice('позвони в экстренную службу')
        ],
    },
}


INCOMING_CALL_TESTS_DATA = {
    'accept_call_no_call': {
        'input_dialog': [
            voice('возьми трубку')
        ],
    },
    'accept_call_has_incoming_call': {
        'input_dialog': [
            voice('возьми трубку', device_state=_make_incoming_call_device_state())
        ],
    },
    'accept_call_has_current_call': {
        'input_dialog': [
            voice('возьми трубку', device_state=_make_current_call_device_state())
        ],
    },
    'decline_call_no_call': {
        'input_dialog': [
            voice('не отвечай')
        ],
    },
    'decline_call_has_incoming_call': {
        'input_dialog': [
            voice('не отвечай', device_state=_make_incoming_call_device_state())
        ],
    },
    'decline_call_has_current_call': {
        'input_dialog': [
            voice('не отвечай', device_state=_make_current_call_device_state())
        ],
    },
    'hangup_call_no_call': {
        'input_dialog': [
            voice('положи трубку')
        ],
    },
    'hangup_call_has_incoming_call': {
        'input_dialog': [
            voice('положи трубку', device_state=_make_incoming_call_device_state())
        ],
    },
    'hangup_call_has_current_call': {
        'input_dialog': [
            voice('положи трубку', device_state=_make_current_call_device_state())
        ],
    },
    'who_calls_no_call': {
        'input_dialog': [
            voice('кто звонит')
        ],
    },
    'who_calls_has_incoming_call': {
        'input_dialog': [
            voice('кто звонит', device_state=_make_incoming_call_device_state())
        ],
    },
    'who_calls_has_current_call': {
        'input_dialog': [
            voice('кто звонит', device_state=_make_current_call_device_state())
        ],
    },
    "who_calls_has_incoming_call_muted": {
        'input_dialog': [
            voice('кто звонит', device_state=_make_incoming_call_device_state(mics_muted=True))
        ],
    },
}


DEVICE_SHORTCUT_TESTS_DATA = {
    'test_shortcut_overlaps_device_calls': {
        'input_dialog': [
            voice(
                'позвони в колонку',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('мышь', 'yandexmini', 'id2')],
                    []
                )
            )
        ],
    },
    'test_shortcut_is_disabled_by_flag': {
        'input_dialog': [
            voice(
                'позвони в колонку',
                request_patcher=_inject_iot_user_info(
                    [_Device('слон', 'irbis', 'id1'), _Device('мышь', 'yandexmini', 'id2')],
                    []
                )
            )
        ],
        'experiments': [
            'hw_disable_device_call_shortcut'
        ]
    }
}


TESTS_DATA = {}
TEST_GEN_PARAMS = []
TEST_RUN_PARAMS = {'argvalues': [], 'ids': []}


def set_custom_experiments(tests_data, experiments):
    for case in tests_data:
        tests_data[case]['experiments'] = experiments

    return tests_data


def add_tests(tests_data, app_presets):
    global TESTS_DATA
    global TEST_GEN_PARAMS
    global TEST_RUN_PARAMS
    TESTS_DATA.update(tests_data)
    TEST_GEN_PARAMS += make_generator_params(tests_data, app_presets)
    test_run_params = make_runner_params(tests_data, app_presets)
    TEST_RUN_PARAMS['argvalues'] += test_run_params['argvalues']
    TEST_RUN_PARAMS['ids'] += test_run_params['ids']


add_tests(INCOMING_CALL_TESTS_DATA, DEFAULT_APP_PRESETS)
add_tests(EMERGENCY_CALL_TESTS_DATA, EMERGENCY_APP_PRESETS)
add_tests(set_custom_experiments(DEVICE_CALL_TESTS_DATA, ['hw_disable_device_call_shortcut']), DEVICE_CALL_APP_PRESETS)
add_tests(DEVICE_SHORTCUT_TESTS_DATA, DEVICE_SHORTCUT_APP_PRESETS)
