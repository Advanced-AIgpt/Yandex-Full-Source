import base64
import json
import logging
import os
import pytest
import signal
import socket
import time
import traceback
from typing import Dict, List, Tuple, Any, Set, Optional

from alice.library.python.eventlog_wrapper.log_dump_record import ApphostAnswerContentType, LogDumpRecord
from alice.megamind.mit.library.graphs_util import GraphsInfo, TBackendTransport, NodeWrapper
from alice.megamind.mit.library.stubber.stubber_repository import StubberRepository
from alice.megamind.mit.library.stubber.http_proxy import HttpProxyService
from alice.megamind.mit.library.util import is_generator_mode
from apphost.api import service
from apphost.python.client.client import Client as ApphostClient, ItemDescriptor


logger = logging.getLogger(__name__)


class ApphostStubberService(object):

    @staticmethod
    def port_count() -> int:
        return 3

    @staticmethod
    def _get_handle_name(source_name: str, request_id: Optional[str]):
        prefix = f'/mit_stubbed_node_{source_name.lower()}'
        if request_id:
            return f'{prefix}_for_reqid_{request_id}'
        return prefix + '_without_reqid'

    def __init__(self, port: int, repository: StubberRepository,
                 graphs_info: GraphsInfo,
                 megamind_nodes: Dict[str, NodeWrapper],
                 test_name: str,
                 unstubbing_subgraph_nodes: Set[str],
                 megamind_port: int,
                 apphost_max_queue_size=1000,
                 apphost_thread_count=10):
        logger.info(f'Initizing apphost stubber service at port {port}')

        self._port: int = port
        self._test_name: str = test_name
        self._thread_count: int = apphost_thread_count
        self._repository: StubberRepository = repository

        self._exceptions: List[str] = []
        self._node_mocks: Dict[str, Any] = dict()
        self._visited_nodes: Set[str] = set()
        self._nodes_should_be_visited: Set[str] = set()
        self._node_input_assertions: Dict[str, Any] = dict()
        self._node_output_assertions: Dict[str, Any] = dict()

        self._graphs_info: GraphsInfo = graphs_info
        self._unstubbing_subgraph_nodes: Set[str] = unstubbing_subgraph_nodes
        self._stubbed_nodes: Dict[Tuple[str, Optional[str]], NodeWrapper] = dict()

        self._megamind_nodes: Dict[str, NodeWrapper] = megamind_nodes
        self._megamind_apphost_client: ApphostClient = ApphostClient('localhost', megamind_port)

        self.__http_proxy: HttpProxyService = HttpProxyService(self.http_proxy_port, self.grpc_port)

        self.__loop: Any = service.Loop(self.__on_shutdown)
        self.__loop.set_max_queue_size(apphost_max_queue_size)

        self._stub_nodes()

    def __enter__(self):
        # We need this dummy handler because
        # we have to have at least one handler on self.port
        # before calling loop.start
        # in runner mode we always have one
        # but in generator mode we do not have stubs so we do not create any handlers before loop.start
        # but we can have mocks in generator mode, and they would not be reachable without dummy handler

        def dummy_handler(_):
            pass

        self.__http_proxy.__enter__()
        self.__loop.add(self.port, dummy_handler, b'/dummy_handler_for_mit')
        self.__loop.enable_grpc(
            self.grpc_port, reuse_port=False, streaming_session_timeout=60 * 1000 * 1000)
        self.__loop.start(thread_count=self._thread_count)
        logger.info(f'Started appohst stubber service at port: {self.port}')
        return self

    def __exit__(self, _type, _value, _traceback):
        logger.info(f'Stopping appohst stubber service at port: {self.port}')
        self._check_necessary_nodes()
        self.__http_proxy.__exit__(_type, _value, _traceback)
        self.__loop.stop()

        if self._exceptions:
            message = 'There is exceptions in stubbed nodes: ' + '\n'.join(self._exceptions)
            pytest.fail(message)

    def __on_shutdown(self):
        logger.info('Got shutdown signal in apphost stubber service')
        os.kill(os.getpid(), signal.SIGINT)

    @property
    def port(self) -> int:
        return self._port

    @property
    def grpc_port(self) -> int:
        return self.port + 1

    @property
    def http_proxy_port(self) -> int:
        return self.port + 2

    @property
    def srcrwrs(self) -> List[str]:
        srcrwr_list: List[str] = []
        for key, node in self._stubbed_nodes.items():
            node_name, request_id = key
            if node.is_backend_use_http_proxy:
                port = self.http_proxy_port
            elif node.backend_transport == TBackendTransport.GRPC:  # type: ignore
                port = self.grpc_port
            else:
                port = self.port
            srcrwr_list.append((f'{node_name}:{socket.gethostname()}:'
                                f'{port}{ApphostStubberService._get_handle_name(node_name, request_id)}'))
        return srcrwr_list

    def _check_necessary_nodes(self):
        def check(do_fail=False) -> bool:
            for necessary_node in self._nodes_should_be_visited:
                if necessary_node not in self._visited_nodes:
                    if do_fail:
                        pytest.fail((f'Node {necessary_node} has not been visited '
                                      'but should have been (added by mock_node or assert_node_input)'))
                    return False
            return True
        time_start = time.time()
        duration_treshold_sec = 5
        sleep_time_sec = 0.1

        #  XXX(yagafarov): This `while True` ugly hack is needed because we wait for APPHOSTSUPPORT-998
        while True:
            if check():
                break
            duration_sec = time.time() - time_start
            if duration_sec > duration_treshold_sec:
                check(do_fail=True)
            time.sleep(sleep_time_sec)

    def get_megamind_unstubbed_nodes(self) -> List[str]:
        return list(
            set(self._megamind_nodes.keys()) -
            set(node_name for node_name, _ in self._stubbed_nodes.keys())
        )

    def _should_be_stubbed(self, record: LogDumpRecord) -> bool:
        if not record.is_source_response() and not record.is_http_source_response():
            return False
        node_name = record.get_source_name()

        if not node_name:
            return False

        if node_name in self._graphs_info.nondefault_nodes:
            logger.info(
                f'Node {node_name} is not stubbed because its type is not DEFAULT')
            return False

        if node_name not in self._graphs_info.nodes.keys():
            raise RuntimeError((f'No node {node_name} in apphost graph, '
                                'probably someone removed it. You should try to recanonize tests'))

        node = self._graphs_info.nodes[node_name]
        if node.name in self._megamind_nodes:
            logger.info(
                f'Node {node_name} is not stubbed because backend is one of megamind')
            return False
        if node.is_inproc_protocol():
            if node_name in self._unstubbing_subgraph_nodes:
                logger.info((f'Node {node_name} is not stubbed because backend is one of subgraphs'
                             f'and node in UNSTUBBING_SUBGRAPH_NODES'))
                return False
            return True
        if node.backend_transport == TBackendTransport.UNKNOWN:  # type: ignore
            logger.info(f'Node {node_name} is not stubbed because backend transport is unknown')
            return False
        return True

    def _stub_nodes(self):
        for request_id, eventlog_wrapper in self._repository.get_eventlog(self._test_name).items():
            for record in eventlog_wrapper.records:
                if not self._should_be_stubbed(record):
                    continue
                source_name = record.get_source_name()
                if not source_name:
                    logger.error(f'No source name for event {record.raw_event}')
                    continue
                self._stub_node(source_name, record, request_id)

    def _stub_node_if_not_yet(self, source_name: str, request_id: Optional[str] = None):
        if not request_id:
            stubbed = source_name in [key[0] for key in self._stubbed_nodes.keys()]
        else:
            stubbed = (source_name, request_id) in self._stubbed_nodes

        if not stubbed:
            self._stub_node(source_name, LogDumpRecord(dict()), request_id)

    def mock_node(self, source_name: str, mock, is_nice_mock=False):
        if not is_nice_mock:
            self._nodes_should_be_visited.add(source_name)
        self._stub_node_if_not_yet(source_name)
        self._node_mocks[source_name] = mock

    def _stub_node_with_assertion(self, source_name: str, is_nice_mock):
        if is_generator_mode():
            return
        if not is_nice_mock:
            self._nodes_should_be_visited.add(source_name)
        self._stub_node_if_not_yet(source_name)

    def assert_node_input(self, source_name: str, assertion, is_nice_mock=False):
        self._stub_node_with_assertion(source_name, is_nice_mock)
        self._node_input_assertions[source_name] = assertion

    def assert_node_output(self, source_name: str, assertion, is_nice_mock=False):
        self._stub_node_with_assertion(source_name, is_nice_mock)
        self._node_output_assertions[source_name] = assertion

    def _stub_node(self, source_name: str, record: LogDumpRecord, request_id: Optional[str]):

        apphost_items: List[Tuple[bytes, bytes, service.ItemType]] = []
        for answer in record.get_answers():
            content = answer.get_content_raw()
            item_name = answer.get_type()
            content_type = answer.get_content_type()
            if not content or not item_name:
                logger.error(
                    f'No content, item name or content_type for {source_name} answer {answer._raw_answer}')
                continue

            if item_name == '__debug_info':
                continue

            if content_type == ApphostAnswerContentType.protobuf:
                assert isinstance(content, str)
                logger.info(
                    f'Prepared protobuf item {item_name} for node {source_name}')
                apphost_items.append((
                    item_name.encode('utf-8'), base64.b64decode(content), service.ItemType.PROTOBUF))
            elif content_type == ApphostAnswerContentType.json:
                assert isinstance(
                    content, dict) or isinstance(content, list)
                logger.info(
                    f'Prepared json item {item_name} for node {source_name}')
                apphost_items.append((
                    item_name.encode('utf-8'), json.dumps(content).encode('utf-8'), service.ItemType.JSON))
            else:
                raise KeyError(f'Unknown content type: {content_type}')

        if record.is_http_source_response():
            http_response = record.get_http_source_response()
            if http_response:
                apphost_items.append((
                    b'http_response', http_response.SerializeToString(), service.ItemType.PROTOBUF))  # type: ignore

        def handler(context):
            try:
                logger.info(f'Handling node {source_name}')

                input_assertion = self._node_input_assertions.get(source_name)
                if input_assertion:
                    input_assertion(context)

                mock = self._node_mocks.get(source_name)
                if mock:
                    mock(context)
                elif source_name in self._megamind_nodes:
                    self._proxy_megamind_request(context, source_name)
                else:
                    for item_name, content, item_type in apphost_items:
                        context.add_raw_item(item_name, content, item_type)

                output_assertion = self._node_output_assertions.get(source_name)
                # TODO(@yagafarov) add ContextAdapter to pass to output assertion
                # for being able to get items via ctx.get_protobuf_item because now there is only ctx.get_raw_item available
                if output_assertion:
                    output_assertion(context)

            except Exception as e:
                message = f'Exception while handling node {source_name}, exception message: {e}, traceback: {traceback.format_exc()}'
                logger.error(message)
                self._exceptions.append(message)
            self._visited_nodes.add(source_name)

        self._stubbed_nodes[(source_name, request_id)] = self._graphs_info.nodes[source_name]
        handle_name = ApphostStubberService._get_handle_name(source_name, request_id)

        self.__http_proxy.add_apphost_handle(handle_name)
        self.__loop.add(self.port, handler, handle_name.encode('utf-8'))
        logger.info(
            f'Successfully stubbed source {source_name} with handle {handle_name} for request_id: {request_id}')

    def _proxy_megamind_request(self, ctx, node_name: str):
        node: NodeWrapper = self._graphs_info.nodes[node_name]
        handler: str = node.handler
        assert handler, 'Trying to send request to megamind node {node_name} with empty handler'
        item_descriptors: List[ItemDescriptor] = list()
        for type_name in ctx.get_protobuf_items_types():
            item_descriptors.append(ItemDescriptor(type_name.decode('utf-8'),
                                                   messages=ctx.get_protobuf_items(type_name)))
        for type_name in ctx.get_raw_input_items_types():
            item_descriptors.append(ItemDescriptor(type_name.decode('utf-8'),
                                                   messages=ctx.get_raw_input_items(type_name)))
        response = self._megamind_apphost_client.request_with_multiple_input_nodes(
            path=handler, item_descriptors=item_descriptors)
        if response.error:
            raise RuntimeError(f'Error while proxying request to megamind node {node_name}: {response.error}')

        for item_type in response.get_all_type_names():
            for content in response.get_type_items(item_type):
                assert isinstance(content, bytes), f'Content from apphost client response should be bytest, got {type(content)}'
                try:
                    content_json = json.loads(content)
                    ctx.add_item(item_type.encode('utf-8'), content_json)
                except Exception:
                    ctx.add_raw_item(item_type.encode('utf-8'), content, service.ItemType.PROTOBUF)
