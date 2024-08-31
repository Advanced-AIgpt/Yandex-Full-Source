# coding: utf-8
import pytest

from alice.megamind.protos.common.device_state_pb2 import TDeviceState


def is_true_message_type(field):
    """ field descriptor is message and not map """
    is_message = field.message_type is not None
    if is_message:
        is_map = field.message_type.GetOptions().map_entry
    else:
        is_map = False

    return is_message and not is_map


def test_all_new_fields_are_not_basic():
    blacklist_fields = {
        'Actions', 'AlarmsState', 'DeviceId', 'InstalledApps',
        'IsDefaultAssistant', 'IsTvPluggedIn', 'MicsMuted', 'SoundLevel',
        'SoundMaxLevel', 'SoundMuted'
    }

    failed_fields = []
    for fd in TDeviceState.DESCRIPTOR.fields:
        if fd.name in blacklist_fields:
            continue

        if not is_true_message_type(fd):
            failed_fields.append(fd.name)

    if failed_fields:
        pytest.fail(f'{TDeviceState.__name__} fields {failed_fields} '
                    'must be a Message type, not a basic type.')
