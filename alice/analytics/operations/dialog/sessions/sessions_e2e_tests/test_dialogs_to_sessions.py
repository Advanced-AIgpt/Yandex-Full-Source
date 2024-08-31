import yatest.common
import json

from nile.api.v1.clusters import MockYQLCluster
from nile.api.v1.local import ListSink
from collections import OrderedDict
from qb2.api.v1 import typing as qt

from alice.analytics.operations.dialog.sessions.lib_for_py2.make_dialog_sessions import make_sessions
from alice.analytics.utils.testing_utils.nile_testing_utils import _resolve_source

DIALOGS_SCHEMA = OrderedDict([
    ('analytics_info', qt.Optional[qt.Yson]),
    ('app_id', qt.Optional[qt.String]),
    ('biometry_classification', qt.Optional[qt.Yson]),
    ('biometry_scoring', qt.Optional[qt.Yson]),
    ('callback_args', qt.Optional[qt.Yson]),
    ('callback_name', qt.Optional[qt.String]),
    ('client_time', qt.Optional[qt.UInt64]),
    ('client_tz', qt.Optional[qt.String]),
    ('contains_sensitive_data', qt.Optional[qt.Bool]),
    ('device_id', qt.Optional[qt.String]),
    ('device_revision', qt.Optional[qt.String]),
    ('dialog_id', qt.Optional[qt.String]),
    ('do_not_use_user_logs', qt.Optional[qt.Bool]),
    ('enrollment_headers', qt.Optional[qt.Yson]),
    ('environment', qt.Optional[qt.String]),
    ('error', qt.Optional[qt.String]),
    ('experiments', qt.Optional[qt.Yson]),
    ('form', qt.Optional[qt.Yson]),
    ('form_name', qt.Optional[qt.String]),
    ('guest_data', qt.Optional[qt.Yson]),
    ('lang', qt.Optional[qt.String]),
    ('location_lat', qt.Optional[qt.Float]),
    ('location_lon', qt.Optional[qt.Float]),
    ('message_id', qt.Optional[qt.String]),
    ('provider', qt.Optional[qt.String]),
    ('puid', qt.Optional[qt.String]),
    ('request', qt.Optional[qt.Yson]),
    ('request_id', qt.Optional[qt.String]),
    ('request_stat', qt.Optional[qt.Yson]),
    ('response', qt.Optional[qt.Yson]),
    ('response_id', qt.Optional[qt.String]),
    ('sequence_number', qt.Optional[qt.UInt64]),
    ('server_time', qt.Optional[qt.UInt64]),
    ('server_time_ms', qt.Optional[qt.UInt64]),
    ('session_id', qt.Optional[qt.String]),
    ('session_status', qt.Optional[qt.Int32]),
    ('trash_or_empty_request', qt.Optional[qt.Bool]),
    ('type', qt.Optional[qt.String]),
    ('utterance_source', qt.Optional[qt.String]),
    ('utterance_text', qt.Optional[qt.String]),
    ('uuid', qt.Optional[qt.String]),
])

DEVICES_SCHEMA = OrderedDict([
    ('app_vers', qt.Optional[qt.String]),
    ('build_type', qt.Optional[qt.String]),
    ('device_id', qt.Optional[qt.String]),
    ('device_type', qt.Optional[qt.String]),
    ('init_date', qt.Optional[qt.String]),
    ('init_timestamp', qt.Optional[qt.UInt64]),
    ('region_id', qt.Optional[qt.Int64]),
    ('subscription_device', qt.Optional[qt.Bool]),
])

USERS_SCHEMA = OrderedDict([
    ('uuid', qt.Optional[qt.String]),
    ('cohort', qt.Optional[qt.String]),
    ('first_day', qt.Optional[qt.String])
])


def test_dialogs_to_sessions(expboxes_path='expboxes.in.json', devices_path='devices.in.json',
                             users_path='users.in.json', sessions_path_expected='sessions.in.json'):
    job = MockYQLCluster().job()
    job = make_sessions(job, 'sessions', 'expboxes', 'users', 'devices', '2021-08-29')

    dir_input = 'test_make_sessions/test_dialogs_to_sessions_input/'
    dir_output = 'test_make_sessions/test_dialogs_to_sessions_output/'

    expboxes_path = yatest.common.runtime.work_path(dir_input + expboxes_path)
    devices_path = yatest.common.runtime.work_path(dir_input + devices_path)
    users_path = yatest.common.runtime.work_path(dir_input + users_path)
    sessions_path_expected = yatest.common.runtime.work_path(dir_output + sessions_path_expected)

    sessions_actual = []

    job.local_run(
        sources={
            'expboxes_table': _resolve_source(expboxes_path, DIALOGS_SCHEMA),
            'devices_table': _resolve_source(devices_path, DEVICES_SCHEMA),
            'users_table': _resolve_source(users_path, USERS_SCHEMA)
        },
        sinks={'sessions_table': ListSink(sessions_actual)},
        allow_remote_requests=False,
    )

    sessions_actual = list(map((lambda record: record.to_dict()), sessions_actual))
    sessions_actual = json.loads(json.dumps(sessions_actual))

    with open(sessions_path_expected, 'r') as sessions_expected_file:
        sessions_expected = json.loads(sessions_expected_file.read())

    assert sorted(sessions_expected) == sorted(sessions_actual)
