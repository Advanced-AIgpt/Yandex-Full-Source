import copy
from google.protobuf import json_format
import logging
import os
from queue import Queue
from typing import Any, Tuple, Generator, Dict, Set, Optional, List

from apphost.daemons.horizon.proto.structures.backend_pb2 import Backend
from apphost.lib.proto_config.types_nora.backend_pb2 import TBackendTransport
from apphost.lib.proto_config.types_nora.graph_pb2 import TNodeType
from apphost.lib.python_util.conf import (
    node_backend_name,
    node_type,
    iterate_graphs_in_dir,
    iterate_graph_nodes,
)


logger = logging.getLogger(__name__)


class NodeWrapper:

    def __init__(self, name: str, node: Any, graph: Any, backend: Any):
        self._name: str = name
        self._node: Any = node
        self._graph: Any = graph
        self._backend = backend

    @property
    def name(self) -> str:
        return self._name

    @property
    def graph_name(self) -> str:
        return self._graph.name

    @property
    def backend_name(self) -> str:
        return node_backend_name(self._node, self._name)

    @property
    def backend_transport(self) -> TBackendTransport:  # type: ignore
        return self._backend.default_settings.transport

    @property
    def is_backend_use_http_proxy(self) -> bool:
        return self._backend.default_settings.http_proxy.value

    @property
    def subgraph_name(self) -> Optional[str]:
        handler: str = self._node.params.handler.value
        parts: List[str] = handler.split('/')
        if '_subhost' not in parts:
            return None
        return parts[-1]

    @property
    def handler(self) -> str:
        return self._node.params.handler.value

    def is_inproc_protocol(self) -> bool:
        return self._backend.default_settings.protocol.value == 'inproc'


class GraphsInfo:

    def __init__(self, nodes: Dict[str, NodeWrapper], nondefault_nodes: Set[str]):
        self._nodes = nodes
        self._nondefault_nodes = nondefault_nodes

    @property
    def nodes(self) -> Dict[str, NodeWrapper]:
        return self._nodes

    @property
    def nondefault_nodes(self) -> Set[str]:
        return self._nondefault_nodes

    def nodes_with_backend(self, backend_name: str) -> Dict[str, NodeWrapper]:
        return dict((name, node) for name, node in self.nodes.items()
                     if node.backend_name == backend_name)


def ah_graphs_info(graphs_dir: str, backends_dir: str, root_graphs_names: Set[str],
                   allowed_subgraph_nodes: Set[str]) -> GraphsInfo:
    backends: Dict[str, Any] = _get_backend_dict_from_dir(backends_dir)
    graphs: Dict[str, Any] = _get_graph_dict_from_dir(graphs_dir)

    ah_nodes: Dict[str, NodeWrapper] = dict()
    nondefault_nodes: Set[str] = set()

    graphs_queue: Any = Queue()

    for graph_name in root_graphs_names:
        graphs_queue.put(graph_name)

    while not graphs_queue.empty():
        graph_name = graphs_queue.get()
        logger.info(f'Walking nodes in graph {graph_name}')
        graph = graphs[graph_name]

        for node_name, node in iterate_graph_nodes(graph):
            backend_name = node_backend_name(node, node_name)
            backend = backends.get(backend_name)
            if node_type(node) not in (TNodeType.DEFAULT, 'DEFAULT'):  # type: ignore
                nondefault_nodes.add(node_name)
                continue
            if not backend:
                raise RuntimeError(f'Node {node_name} is DEFAULT but have no backend')
            backend = _merge_backend_settings_from_all_parents(backend, backends)
            node_wrapper = NodeWrapper(node_name, node, graph, backend)
            if node_wrapper.is_inproc_protocol and node_name in allowed_subgraph_nodes:
                subgraph_name = node_wrapper.subgraph_name
                if not subgraph_name:
                    raise KeyError(f'No subgraph name for inproc node {node_name}')
                graphs_queue.put(subgraph_name)
            ah_nodes[node_name] = node_wrapper
            logger.debug(f'Added node {node_name}')

    return GraphsInfo(ah_nodes, nondefault_nodes)


def _merge_backend_settings_from_all_parents(initial_backend, backends: Dict[str, Any]):
    backends_list = [initial_backend]
    current_backend = initial_backend
    while current_backend.inherits:
        current_backend = backends[current_backend.inherits]
        backends_list.append(current_backend)
    backends_list.reverse()

    result_backend = copy.deepcopy(backends_list[0])
    for backend in backends_list:
        result_backend.MergeFrom(backend)
    return result_backend


def _proto_from_json_str(proto_type, json_str: str):
    return json_format.Parse(json_str, proto_type(), ignore_unknown_fields=True)


def _read_backend(backend_path: str) -> Any:
    assert os.path.isfile(backend_path)

    with open(backend_path, 'r') as graph_file:
        return _proto_from_json_str(Backend, graph_file.read())


def _is_backend_filename(filename: str) -> bool:
    return filename.endswith('.json')


def _iterate_backends_in_dir(backeds_dir: str) -> Generator[Tuple[str, Any], None, None]:
    assert os.path.isdir(backeds_dir)

    for filename in os.listdir(backeds_dir):
        path = os.path.join(backeds_dir, filename)

        if os.path.isdir(path):
            for path, backend in _iterate_backends_in_dir(path):
                yield path, backend

        elif os.path.isfile(path):
            if _is_backend_filename(filename):
                backend = _read_backend(path)
                yield path, backend


def _get_backend_dict_from_dir(backends_dir: str) -> Dict[str, Any]:
    backends: Dict[str, Any] = dict()
    for path, backend in _iterate_backends_in_dir(backends_dir):
        backend_name = os.path.relpath(path, start=backends_dir).replace(
            '.json', '').replace('/', '__')
        backends[backend_name] = backend
    return backends


def _graph_name(graph_path: str):
    _, file_name = os.path.split(graph_path)
    return file_name.replace('.json', '')


def _get_graph_dict_from_dir(graphs_dir: str) -> Dict[str, Any]:
    graphs: Dict[str, Any] = dict()
    for graph, path in iterate_graphs_in_dir(graphs_dir):
        # I dk why initally graph.name is empty
        graph.name = _graph_name(path)
        graphs[graph.name] = graph
    return graphs
