import pytest
import uuid

from .iot_mock import IoTMock
from .matrix import Matrix
from .python_notificator import PythonNotificator
from .subway_mock import SubwayMock

from alice.matrix.library.testing.protos.incompatible_pb2 import TIncompatibleObject

from alice.matrix.library.testing.python.tornado_io_loop import TornadoIOLoop
from alice.matrix.library.testing.python.tvm import (
    get_test_tvm_api_clients_info,
    get_tvm_api_port,

    get_tvmtool_authtoken,
    get_tvmtool_port,
)
from alice.matrix.library.testing.python.ydb import create_ydb_session


class MatrixTestBase:
    matrix_disable_ydb_operations_in_locator_service = False
    matrix_pushes_and_notifications_mock_mode = False
    matrix_user_white_list = None

    @pytest.fixture(scope="class")
    def iot_mock(self):
        with IoTMock() as i:
            yield i

    @pytest.fixture(scope="function", autouse=True)
    def reset_iot_mock(self, iot_mock):
        # It is very expensive to restart iot for each test
        # so, just reset it
        iot_mock.reset()

    @pytest.fixture(scope="class")
    def subway_mock(self):
        with SubwayMock() as s:
            yield s

    @pytest.fixture(scope="function", autouse=True)
    def reset_subway_mock(self, subway_mock):
        # It is very expensive to restart subway for each test
        # so, just reset it
        subway_mock.reset()

    @pytest.fixture(scope="class", autouse=True)
    def tornado_io_loop(
        self,

        # Start tornado ioloop after adding all applications to it
        iot_mock,
        subway_mock,
    ):
        with TornadoIOLoop() as l:
            yield l

    @pytest.fixture(scope="class")
    def tvm_api_clients_info(self):
        return get_test_tvm_api_clients_info(
            destinations=[
                "iot",
            ],
        )

    @pytest.fixture(scope="class")
    def tvm_api_port(self):
        return get_tvm_api_port()

    @pytest.fixture(scope="class")
    def tvmtool_authtoken(self):
        return get_tvmtool_authtoken()

    @pytest.fixture(scope="class")
    def tvmtool_port(self):
        return get_tvmtool_port()

    @pytest.fixture(scope="class")
    def python_notificator(
        self,

        tvm_api_clients_info,
        tvm_api_port,

        iot_mock,
        subway_mock,
    ):
        with PythonNotificator(
            tvm_api_port=tvm_api_port,
            tvm_client_id=tvm_api_clients_info["self"]["id"],
            tvm_client_secret=tvm_api_clients_info["self"]["secret"],

            iot_port=iot_mock.port,
            iot_tvm_client_id=tvm_api_clients_info["destinations"]["iot"],

            subway_port=subway_mock.port,

            pushes_and_notifications_mock_mode=self.matrix_pushes_and_notifications_mock_mode,
            user_white_list=self.matrix_user_white_list,
        ) as n:
            yield n

    @pytest.fixture(scope="class")
    def matrix(
        self,

        tvmtool_authtoken,
        tvmtool_port,

        iot_mock,
        python_notificator,
        subway_mock,
    ):
        with Matrix(
            tvmtool_authtoken=tvmtool_authtoken,
            tvmtool_port=tvmtool_port,

            iot_port=iot_mock.port,
            python_notificator_port=python_notificator._http_port,
            subway_port=subway_mock.port,

            disable_ydb_operations_in_locator_service=self.matrix_disable_ydb_operations_in_locator_service,
            pushes_and_notifications_mock_mode=self.matrix_pushes_and_notifications_mock_mode,
            user_white_list=self.matrix_user_white_list,
        ) as m:
            yield m

    @pytest.fixture(scope="class")
    def ydb_session(self):
        return create_ydb_session()

    @pytest.fixture(scope="function")
    def puid(self):
        return f"puid_{uuid.uuid4()}"

    @pytest.fixture(scope="function")
    def device_id(self):
        return f"device_id_{uuid.uuid4()}"


class MatrixAppHostSingleItemHandlerTestBase(MatrixTestBase):
    path = None
    request_item_type = None
    response_item_type = None
    simple_request_proto = None
    response_proto_type = None

    simple_ok_response_checker = None

    async def _perform_matrix_request(self, matrix, request_proto, timeout=3):
        return await matrix.perform_grpc_request(
            items={
                self.request_item_type: [
                    request_proto
                ]
            },
            path=self.path,
            timeout=timeout,
        )

    async def _perform_matrix_request_with_check(self, matrix, request_proto, timeout=3):
        response = await self._perform_matrix_request(matrix, request_proto, timeout)
        assert not response.has_exception(), f"Request failed with exception: {response.get_exception()}"

        response_items = self._get_response_item_datas(response)
        assert len(response_items) == 1
        return response_items[0]

    def _get_response_item_datas(self, response):
        return list(response.get_item_datas(item_type=self.response_item_type, proto_type=self.response_proto_type))

    @pytest.mark.asyncio
    async def test_base_class_fields(self):
        assert self.path is not None
        assert self.request_item_type is not None
        assert self.response_proto_type is not None
        assert self.simple_request_proto is not None
        assert self.response_proto_type is not None

    @pytest.mark.asyncio
    async def test_simple_ok(self, matrix):
        response_proto = await self._perform_matrix_request_with_check(matrix, self.simple_request_proto)
        if self.simple_ok_response_checker is not None:
            self.simple_ok_response_checker(response_proto)

    @pytest.mark.asyncio
    async def test_simple_error(self, matrix):
        # Missing item
        response = await matrix.perform_grpc_request(
            items={
                "incorrect_item_name": [
                    self.simple_request_proto
                ]
            },
            path=self.path,
            timeout=3,
        )
        assert len(self._get_response_item_datas(response)) == 0
        assert response.has_exception()
        assert b"not found in request" in response.get_exception()

        # Incorrect protobuf
        incompatible_proto = TIncompatibleObject()
        incompatible_proto.First[:] = incompatible_proto.Second[:] = ["qwe", "abc", "asd"]
        response = await matrix.perform_grpc_request(
            items={
                self.request_item_type: [
                    incompatible_proto
                ]
            },
            path=self.path,
            timeout=3,
        )
        assert len(self._get_response_item_datas(response)) == 0
        assert response.has_exception()
        assert b"Cant parse field" in response.get_exception()
        assert b"as protobuf" in response.get_exception()

        # More than one item
        response = await matrix.perform_grpc_request(
            items={
                self.request_item_type: [
                    self.simple_request_proto,
                    self.simple_request_proto
                ]
            },
            path=self.path,
            timeout=3,
        )
        assert len(self._get_response_item_datas(response)) == 0
        assert response.has_exception()
        assert b"has more than one element" in response.get_exception()
