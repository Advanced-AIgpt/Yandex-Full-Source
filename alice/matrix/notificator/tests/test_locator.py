import pytest
import os

from alice.matrix.notificator.tests.library.test_base import MatrixTestBase

from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_device_locator,
    get_get_devices,
)

import ydb


def _get_user_devices_from_ydb(ydb_session, puid):
    table_prefix = os.getenv("YDB_DATABASE")
    query = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $puid AS String;

        SELECT device_id, host, ts, device_model, config
            FROM device_locator
            WHERE puid = $puid
    """.format(table_prefix)
    prepared_query = ydb_session.prepare(query)
    parameters = {
        "$puid": puid.encode("utf-8"),
    }

    res = ydb_session.transaction(ydb.OnlineReadOnly()).execute(prepared_query, parameters, commit_tx=True)

    result = []
    for row in res[0].rows:
        result.append({
            "device_id": row.device_id.decode("utf-8"),
            "host": row.host.decode("utf-8"),
            "ts": row.ts,
            "device_model": row.device_model.decode("utf-8"),
            "config": row.config,
        })

    return result


class TestLocator(MatrixTestBase):

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        ydb_session,
        puid,
        device_id,
    ):
        # Add device
        locator_request = get_device_locator(puid, device_id, supported_features=["Hello", "World"])
        matrix.perform_locator_add_request(locator_request)

        # Get device
        get_devices_response = matrix.perform_get_devices_request(get_get_devices(puid))
        assert len(get_devices_response.Devices) == 1
        assert get_devices_response.Devices[0].DeviceId == device_id
        assert not get_devices_response.Devices[0].SupportedFeatures

        # Check ydb's content
        assert _get_user_devices_from_ydb(ydb_session, puid) == [
            {
                "device_id": device_id,
                "host": locator_request.Host,
                "ts": locator_request.Timestamp,
                "device_model": locator_request.DeviceModel,
                "config": locator_request.Config.SerializeToString(),
            },
        ]

        # Delete device
        locator_request.Timestamp += 1
        matrix.perform_locator_remove_request(locator_request)

        # Get device
        get_devices_response = matrix.perform_get_devices_request(get_get_devices(puid))
        assert not get_devices_response.Devices


class TestLocatorWithDisabledYDBOperations(MatrixTestBase):
    matrix_disable_ydb_operations_in_locator_service = True

    @pytest.mark.asyncio
    async def test_simple(
        self,
        matrix,
        ydb_session,
        puid,
        device_id,
    ):
        # Add device
        locator_request = get_device_locator(puid, device_id, supported_features=["Hello", "World"])
        matrix.perform_locator_add_request(locator_request)

        # Get device
        get_devices_response = matrix.perform_get_devices_request(get_get_devices(puid))
        assert not get_devices_response.Devices

        # Check ydb's content
        assert _get_user_devices_from_ydb(ydb_session, puid) == []

        # Delete device
        locator_request.Timestamp += 1
        matrix.perform_locator_remove_request(locator_request)

        # Get device
        get_devices_response = matrix.perform_get_devices_request(get_get_devices(puid))
        assert not get_devices_response.Devices
