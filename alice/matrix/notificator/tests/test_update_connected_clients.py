import pytest
import os
import uuid

from random import randrange

from alice.matrix.notificator.tests.library.constants import ItemTypes, ServiceHandlers
from alice.matrix.notificator.tests.library.test_base import MatrixAppHostSingleItemHandlerTestBase

from alice.protos.api.matrix.client_connections_pb2 import (
    TUpdateConnectedClientsResponse,
)
from alice.protos.api.matrix.user_device_pb2 import (
    TUserDeviceInfo,
)
from alice.megamind.protos.speechkit.directives_pb2 import (
    TDirective as TSpeechKitDirective,
)
from alice.matrix.notificator.tests.library.proto_builder_helpers import (
    get_client_state_change_connect,
    get_client_state_change_disconnect,
    get_internal_device_info,
    get_random_uniproxy_endpoint,
    get_update_connected_clients_request,
    get_user_device_info,

    get_delivery,
)

from alice.matrix.notificator.tests.library.utils import get_multi_hash_ui64

from alice.matrix.library.testing.python.ydb import create_ydb_session
from cityhash import hash64 as CityHash64
from google.protobuf import json_format

import ydb


def _get_random_shard_id():
    return randrange(0, 10001)


def _get_shard_key(ip, ip_shard_id):
    return get_multi_hash_ui64(CityHash64(ip.encode("utf-8")), ip_shard_id)


def _get_connections_for_ip_shard(ydb_session, ip, ip_shard_id):
    limit = 1000

    table_prefix = os.getenv("YDB_DATABASE")
    query_no_last = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $limit AS Uint64;
        DECLARE $shard_key AS Uint64;
        DECLARE $ip AS String;
        DECLARE $ip_shard_id AS Uint64;

        SELECT * FROM connections_sharded
        WHERE
            shard_key = $shard_key AND
            ip = $ip AND
            ip_shard_id = $ip_shard_id
        ORDER BY shard_key, ip, puid, device_id
        LIMIT $limit
    """.format(table_prefix)
    query_with_last = """
        PRAGMA TablePathPrefix("{}");

        DECLARE $limit AS Uint64;
        DECLARE $shard_key AS Uint64;
        DECLARE $ip AS String;
        DECLARE $ip_shard_id AS Uint64;
        DECLARE $last_puid AS String;
        DECLARE $last_device_id AS String;

        $part1 = (
            SELECT * FROM connections_sharded
            WHERE
                shard_key = $shard_key AND
                ip = $ip AND
                ip_shard_id = $ip_shard_id AND
                puid = $last_puid AND
                device_id > $last_device_id
            ORDER BY shard_key, ip, puid, device_id
            LIMIT $limit
        );

        $part2 = (
            SELECT * FROM connections_sharded
            WHERE
                shard_key = $shard_key AND
                ip = $ip AND
                ip_shard_id = $ip_shard_id AND
                puid > $last_puid
            ORDER BY shard_key, ip, puid, device_id
            LIMIT $limit
        );

        SELECT * FROM (
            SELECT * FROM $part1
            UNION ALL
            SELECT * FROM $part2
        )
        ORDER BY shard_key, ip, puid, device_id
        LIMIT $limit
    """.format(table_prefix)

    def _get_page(last_puid=None, last_device_id=None):
        parameters = {
            "$limit": limit,
            "$shard_key": _get_shard_key(ip, ip_shard_id),
            "$ip": ip.encode("utf-8"),
            "$ip_shard_id": ip_shard_id,
        }

        if last_puid is None and last_device_id is None:
            prepared_query = ydb_session.prepare(query_no_last)
            res = ydb_session.transaction(ydb.OnlineReadOnly()).execute(prepared_query, parameters, commit_tx=True)
        else:
            assert last_puid is not None and last_device_id is not None
            prepared_query = ydb_session.prepare(query_with_last)
            parameters.update({
                "$last_puid": last_puid.encode("utf-8"),
                "$last_device_id": last_device_id.encode("utf-8"),
            })

            res = ydb_session.transaction(ydb.OnlineReadOnly()).execute(prepared_query, parameters, commit_tx=True)

        return [
            {
                "shard_key": row.shard_key,
                "ip": row.ip.decode("utf-8"),
                "ip_shard_id": row.ip_shard_id,
                "puid": row.puid.decode("utf-8"),
                "device_id": row.device_id.decode("utf-8"),

                "port": row.port,
                "monotonic": row.monotonic,
                "device_info": row.device_info,

                "created_at": row.created_at,
                "expired_at": row.expired_at,
            }
            for row in res[0].rows
        ]

    last_puid = None
    last_device_id = None
    result = []
    while True:
        current_result = _get_page(last_puid, last_device_id)
        result.extend(current_result)
        if len(current_result) == limit:
            last_puid, last_device_id = result[-1]["puid"], result[-1]["device_id"]
        else:
            break

    return result


def _check_connections_for_uniproxy_endpoint(
    ydb_session,
    uniproxy_endpoint,
    shard_id,
    expected_connections,
    check_device_info=False,
):
    ip = uniproxy_endpoint.Ip
    port = uniproxy_endpoint.Port
    shard_key = _get_shard_key(ip, shard_id)

    connections_full_info = _get_connections_for_ip_shard(ydb_session, ip, shard_id)
    connections = set()
    for connection in connections_full_info:
        assert connection["shard_key"] == shard_key
        assert connection["ip"] == ip
        assert connection["ip_shard_id"] == shard_id

        assert connection["port"] == port
        assert connection["created_at"] is not None
        assert connection["expired_at"] is not None

        connections.add(
            tuple(connection[key] for key in ["puid", "device_id", "monotonic"] + (["device_info"] if check_device_info else []))
        )

    assert connections == expected_connections


class TestUpdateConnectedClients(MatrixAppHostSingleItemHandlerTestBase):
    path = ServiceHandlers.UPDATE_CONNECTED_CLIENTS
    request_item_type = ItemTypes.CONNECTED_CLIENTS_UPDATE_REQUEST
    response_item_type = ItemTypes.CONNECTED_CLIENTS_UPDATE_RESPONSE
    simple_request_proto = get_update_connected_clients_request(
        get_random_uniproxy_endpoint(),
        12354,
        _get_random_shard_id(),
        client_state_changes=[
            get_client_state_change_connect(get_user_device_info("1234", "device_id_1", "station")),
            get_client_state_change_connect(get_user_device_info("94585", "device_id_3", "station_ff")),
            get_client_state_change_disconnect(get_user_device_info("1234", "device_id_1", "station")),
            get_client_state_change_disconnect(get_user_device_info("4321", "device_id_2", "station_2")),

            get_client_state_change_connect(get_user_device_info("", "no_puid", "station")),
            get_client_state_change_connect(get_user_device_info("no_device_id", "", "station")),
            get_client_state_change_connect(get_user_device_info("", "", "no_puid_and_device_id")),
        ],
        clients_full_state=[
            get_user_device_info("12345", "device_id_4", "station"),
            get_user_device_info("9485", "device_id_3", "station_ff"),

            get_user_device_info("", "no_puid", "station"),
            get_user_device_info("no_device_id", "", "station"),
            get_user_device_info("", "", "no_puid_and_device_id"),
        ],
        all_connections_dropped_on_shutdown=False,
    )
    response_proto_type = TUpdateConnectedClientsResponse

    def simple_ok_response_checker(self, response):
        _check_connections_for_uniproxy_endpoint(
            create_ydb_session(),
            self.simple_request_proto.UniproxyEndpoint,
            self.simple_request_proto.ShardId,
            {
                ("9485", "device_id_3", 12354, get_internal_device_info("station_ff").SerializeToString()),
                ("12345", "device_id_4", 12354, get_internal_device_info("station").SerializeToString()),
            },
            check_device_info=True,
        )

    @pytest.fixture(scope="function")
    def uniproxy_endpoint(self):
        return get_random_uniproxy_endpoint()

    @pytest.fixture(scope="function")
    def shard_id(self):
        return _get_random_shard_id()

    @pytest.mark.asyncio
    async def test_simple_add_remove(self, matrix, ydb_session, uniproxy_endpoint, shard_id):
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p1", "d1")),
                    get_client_state_change_connect(get_user_device_info("p2", "d2")),
                    get_client_state_change_connect(get_user_device_info("p3", "d3")),
                ],
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 1),
                ("p2", "d2", 1),
                ("p3", "d3", 1),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p1", "d1")),
                    get_client_state_change_connect(get_user_device_info("p4", "d4")),
                    get_client_state_change_disconnect(get_user_device_info("p2", "d2")),
                ],
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 2),
                ("p3", "d3", 1),
                ("p4", "d4", 2),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                3,
                shard_id,
                client_state_changes=[
                    get_client_state_change_disconnect(get_user_device_info("p3", "d3")),
                ],
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 2),
                ("p4", "d4", 2),
            },
        )

    @pytest.mark.asyncio
    async def test_add_and_remove_in_same_request(self, matrix, uniproxy_endpoint, ydb_session, shard_id):
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p8", "d8")),
                    get_client_state_change_connect(get_user_device_info("p9", "d9")),
                ],
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p8", "d8", 1),
                ("p9", "d9", 1),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p1", "d1")),
                    get_client_state_change_disconnect(get_user_device_info("p1", "d1")),

                    # (p2, d2) is not present in database and should not be added (no request for disconnect, connect)
                    get_client_state_change_disconnect(get_user_device_info("p2", "d2")),
                    get_client_state_change_connect(get_user_device_info("p2", "d2")),

                    get_client_state_change_connect(get_user_device_info("p3", "d3")),
                    get_client_state_change_disconnect(get_user_device_info("p3", "d3")),
                    get_client_state_change_connect(get_user_device_info("p3", "d3")),
                    get_client_state_change_disconnect(get_user_device_info("p3", "d3")),
                    get_client_state_change_connect(get_user_device_info("p3", "d3")),

                    get_client_state_change_disconnect(get_user_device_info("p4", "d4")),
                    get_client_state_change_connect(get_user_device_info("p4", "d4")),
                    get_client_state_change_disconnect(get_user_device_info("p4", "d4")),

                    # Something went wrong here
                    get_client_state_change_disconnect(get_user_device_info("p5", "d5")),
                    get_client_state_change_disconnect(get_user_device_info("p5", "d5")),

                    # Something went wrong here
                    get_client_state_change_connect(get_user_device_info("p6", "d6")),
                    get_client_state_change_connect(get_user_device_info("p6", "d6")),

                    # Something went wrong here
                    get_client_state_change_disconnect(get_user_device_info("p7", "d7")),
                    get_client_state_change_disconnect(get_user_device_info("p7", "d7")),
                    get_client_state_change_connect(get_user_device_info("p7", "d7")),
                    get_client_state_change_connect(get_user_device_info("p7", "d7")),

                    # (p8, d8) is present in database and should not be removed (no request for connect, disconnect)
                    get_client_state_change_connect(get_user_device_info("p8", "d8")),
                    get_client_state_change_disconnect(get_user_device_info("p8", "d8")),

                    # Something went wrong here
                    get_client_state_change_connect(get_user_device_info("p9", "d9")),
                    get_client_state_change_connect(get_user_device_info("p9", "d9")),
                    get_client_state_change_disconnect(get_user_device_info("p9", "d9")),
                    get_client_state_change_disconnect(get_user_device_info("p9", "d9")),
                ],
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p3", "d3", 2),
                ("p6", "d6", 2),
                ("p7", "d7", 2),
                ("p8", "d8", 1),
            },
        )

    @pytest.mark.asyncio
    async def test_shards_independence(self, matrix, uniproxy_endpoint, ydb_session, shard_id):
        first_shard_id = shard_id
        second_shard_id = shard_id + 1

        # Lets add some connections within first shard
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                first_shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p1", "d1")),
                    get_client_state_change_connect(get_user_device_info("p2", "d2")),
                ],
            ),
        )

        # These two connections should be added to connections table within first shard
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            first_shard_id,
            {
                ("p1", "d1", 1),
                ("p2", "d2", 1),
            },
        )

        # But nothing should be found if we seek for connections within second shard
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            second_shard_id,
            set(),
        )

        # Lets now add some connections within second shard
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                second_shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p3", "d3")),
                    get_client_state_change_connect(get_user_device_info("p4", "d4")),
                ],
            ),
        )

        # These two connections should be added to connections table within second shard
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            second_shard_id,
            {
                ("p3", "d3", 2),
                ("p4", "d4", 2),
            },
        )

        # But they must not affect connections within first shard
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            first_shard_id,
            {
                ("p1", "d1", 1),
                ("p2", "d2", 1),
            },
        )

        # Lets now remove a connection within first shard
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                3,
                first_shard_id,
                client_state_changes=[
                    get_client_state_change_disconnect(get_user_device_info("p1", "d1")),
                ],
            ),
        )

        # The connection should be removed within first shard
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            first_shard_id,
            {
                ("p2", "d2", 1),
            },
        )

        # The connection should be removed within first shard
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            second_shard_id,
            {
                ("p3", "d3", 2),
                ("p4", "d4", 2),
            },
        )

        # Lets try to disconnect (p2, d2) within shard id 2
        # (it was connected within shard id 1, so nothing should happen here)
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                4,
                second_shard_id,
                client_state_changes=[
                    get_client_state_change_disconnect(get_user_device_info("p2", "d2")),
                ],
            ),
        )

        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            first_shard_id,
            {
                ("p2", "d2", 1),
            },
        )

        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            second_shard_id,
            {
                ("p3", "d3", 2),
                ("p4", "d4", 2),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                5,
                first_shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info("p1", "d1"),
                    get_user_device_info("p5", "d5"),
                ]
            ),
        )

        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            first_shard_id,
            {
                ("p1", "d1", 5),
                ("p5", "d5", 5),
            },
        )

        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            second_shard_id,
            {
                ("p3", "d3", 2),
                ("p4", "d4", 2),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                6,
                second_shard_id,
                client_state_changes=[],
                clients_full_state=None,
                all_connections_dropped_on_shutdown=True,
            ),
        )

        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            first_shard_id,
            {
                ("p1", "d1", 5),
                ("p5", "d5", 5),
            },
        )

        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            second_shard_id,
            set(),
        )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("reason", ["shutdown", "empty_state", "shutdown_and_empty_state"])
    async def test_all_connections_dropped(self, matrix, uniproxy_endpoint, ydb_session, shard_id, reason):
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p1", "d1")),
                    get_client_state_change_connect(get_user_device_info("p2", "d2")),
                    get_client_state_change_connect(get_user_device_info("p3", "d3")),
                ],
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 1),
                ("p2", "d2", 1),
                ("p3", "d3", 1),
            },
        )

        client_state_changes = [
            get_client_state_change_connect(get_user_device_info("p4", "d4")),
        ]
        clients_full_state = [] if reason == "empty_state" or reason == "shutdown_and_empty_state" else None
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                shard_id,
                client_state_changes=client_state_changes,
                clients_full_state=clients_full_state,
                all_connections_dropped_on_shutdown=(reason == "shutdown" or reason == "shutdown_and_empty_state"),
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            set(),
        )

    @pytest.mark.asyncio
    async def test_update_from_full_state(self, matrix, uniproxy_endpoint, ydb_session, shard_id):
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info("p1", "d1"),
                    get_user_device_info("p2", "d2"),
                    get_user_device_info("p3", "d3"),
                ]
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 1),
                ("p2", "d2", 1),
                ("p3", "d3", 1),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info("p2", "d2"),
                    get_user_device_info("p4", "d4"),
                    get_user_device_info("p5", "d5"),
                ]
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                # p1 and p3 are removed
                # p2 is not updated (monotonic = 1)
                ("p2", "d2", 1),

                # p4 and p5 are added
                ("p4", "d4", 2),
                ("p5", "d5", 2),
            },
        )

    @pytest.mark.asyncio
    async def test_update_from_full_state_with_diff_update(self, matrix, uniproxy_endpoint, ydb_session, shard_id):
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info("p1", "d1"),
                    get_user_device_info("p2", "d2"),
                    get_user_device_info("p3", "d3"),
                ]
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 1),
                ("p2", "d2", 1),
                ("p3", "d3", 1),
            },
        )

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                shard_id,
                client_state_changes=[
                    get_client_state_change_connect(get_user_device_info("p2", "d2")),
                    get_client_state_change_connect(get_user_device_info("p6", "d6")),
                ],
                clients_full_state=[
                    get_user_device_info("p2", "d2"),
                    get_user_device_info("p4", "d4"),
                    get_user_device_info("p5", "d5"),
                ]
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                # p1 and p3 are removed
                # p2 is updated by diff (so monotonic = 2)
                ("p2", "d2", 2),
                # p6 is added by diff, but removed by full state

                # p4 and p5 are added
                ("p4", "d4", 2),
                ("p5", "d5", 2),
            },
        )

    @pytest.mark.asyncio
    async def test_big_update_from_full_state(self, matrix, uniproxy_endpoint, ydb_session, shard_id):
        CONNECTIONS_COUNT = 5000

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info(f"p{i}", f"d{i}")
                    for i in range(CONNECTIONS_COUNT)
                ]
            ),
            timeout=10,
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                (f"p{i}", f"d{i}", 1)
                for i in range(CONNECTIONS_COUNT)
            },
        )

        # Small update
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                2,
                shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info(f"p{i}", f"d{i}")
                    for i in range(CONNECTIONS_COUNT)
                ] + [
                    get_user_device_info("p10", "d10_new"),
                    get_user_device_info("p100", "d100_new"),
                    get_user_device_info("p1000", "d1000_new"),
                ]
            ),
            timeout=10,
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                (f"p{i}", f"d{i}", 1)
                for i in range(CONNECTIONS_COUNT)
            } | {
                ("p10", "d10_new", 2),
                ("p100", "d100_new", 2),
                ("p1000", "d1000_new", 2),
            },
        )

        # Change all
        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                3,
                shard_id,
                client_state_changes=[],
                clients_full_state=[
                    get_user_device_info(f"p{i}", f"d{i + 1}")
                    for i in range(CONNECTIONS_COUNT)
                ]
            ),
            timeout=10,
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                (f"p{i}", f"d{i + 1}", 3)
                for i in range(CONNECTIONS_COUNT)
            },
        )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("update_type", ["diff", "full_state"])
    async def test_device_info_from_update(self, matrix, uniproxy_endpoint, ydb_session, update_type, shard_id):
        client_state_changes = []
        clients_full_state = None
        clients_user_device_info = [
            get_user_device_info("p1", "d1", "s1", []),
            get_user_device_info("p2", "d2", "s2", [TUserDeviceInfo.ESupportedFeature.AUDIO_CLIENT]),
            get_user_device_info("p3", "d3", "s3", [TUserDeviceInfo.ESupportedFeature.UNKNOWN, TUserDeviceInfo.ESupportedFeature.AUDIO_CLIENT]),
        ]
        if update_type == "diff":
            client_state_changes = [
                get_client_state_change_connect(user_device_info)
                for user_device_info in clients_user_device_info
            ]
        elif update_type == "full_state":
            clients_full_state = clients_user_device_info
        else:
            assert False, f"Unknown update type {update_type}"

        await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=client_state_changes,
                clients_full_state=clients_full_state,
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                ("p1", "d1", 1, get_internal_device_info("s1", []).SerializeToString()),
                ("p2", "d2", 1, get_internal_device_info("s2", [TUserDeviceInfo.ESupportedFeature.AUDIO_CLIENT]).SerializeToString()),
                ("p3", "d3", 1, get_internal_device_info("s3", [TUserDeviceInfo.ESupportedFeature.UNKNOWN, TUserDeviceInfo.ESupportedFeature.AUDIO_CLIENT]).SerializeToString()),
            },
            check_device_info=True,
        )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("update_type", ["diff", "full_state"])
    async def test_get_directives_for_connected_clients(self, matrix, uniproxy_endpoint, ydb_session, update_type, shard_id):
        client_state_changes = []
        clients_full_state = None
        clients_user_device_info = [
            get_user_device_info(f"p1_{uuid.uuid4()}", "d1"),
            get_user_device_info(f"p2_{uuid.uuid4()}", "d2"),
            get_user_device_info(f"p3_{uuid.uuid4()}", "d3"),
        ]
        if update_type == "diff":
            client_state_changes = [
                get_client_state_change_connect(user_device_info)
                for user_device_info in clients_user_device_info
            ]
        elif update_type == "full_state":
            clients_full_state = clients_user_device_info
        else:
            assert False, f"Unknown update type {update_type}"

        cnt_directives_per_client = [
            2,
            3,
            0,
        ]
        for i in range(len(cnt_directives_per_client)):
            for j in range(cnt_directives_per_client[i]):
                puid, device_id = clients_user_device_info[i].UserDeviceIdentifier.Puid, clients_user_device_info[i].UserDeviceIdentifier.DeviceId
                matrix.perform_post_request(
                    "/delivery/push",
                    data=get_delivery(
                        puid=puid,
                        device_id=device_id,
                        push_id=f"{puid}-{device_id}-{j}",
                    ).SerializeToString()
                )

        response = await self._perform_matrix_request_with_check(
            matrix,
            get_update_connected_clients_request(
                uniproxy_endpoint,
                1,
                shard_id,
                client_state_changes=client_state_changes,
                clients_full_state=clients_full_state,
            ),
        )
        _check_connections_for_uniproxy_endpoint(
            ydb_session,
            uniproxy_endpoint,
            shard_id,
            {
                (user_device_info.UserDeviceIdentifier.Puid, user_device_info.UserDeviceIdentifier.DeviceId, 1)
                for user_device_info in clients_user_device_info
            },
        )

        technical_pushes_for_user_devices = list(response.TechnicalPushesForUserDevices)
        technical_pushes_for_user_devices.sort(key=lambda x: x.UserDeviceIdentifier.Puid)
        assert len(technical_pushes_for_user_devices) == 2
        for i in range(2):
            technical_pushes_for_user_device = technical_pushes_for_user_devices[i]
            assert technical_pushes_for_user_device.UserDeviceIdentifier == clients_user_device_info[i].UserDeviceIdentifier
            puid = technical_pushes_for_user_device.UserDeviceIdentifier.Puid
            device_id = technical_pushes_for_user_device.UserDeviceIdentifier.DeviceId

            technical_pushes = list(technical_pushes_for_user_device.TechnicalPushes)
            technical_pushes.sort(key=lambda x: x.TechnicalPushId)
            assert len(technical_pushes) == cnt_directives_per_client[i]
            for i in range(len(technical_pushes)):
                assert technical_pushes[i].TechnicalPushId == f"{puid}-{device_id}-{i}"

                speech_kit_directive = TSpeechKitDirective()
                technical_pushes[i].SpeechKitDirective.Unpack(speech_kit_directive)
                assert json_format.MessageToDict(speech_kit_directive) == {
                    "type": "server_action",
                    "name": "@@mm_semantic_frame",
                    "payload": {
                        "analytics": {
                            "origin": "Scenario",
                            "purpose": "sound_set_level"
                        },
                        "typed_semantic_frame": {
                            "sound_set_level_semantic_frame": {
                                "level": {
                                    "num_level_value": 0.0
                                }
                            }
                        }
                    }
                }
