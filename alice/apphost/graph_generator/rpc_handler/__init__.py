import logging
from pathlib import Path

import alice.apphost.graph_generator.template as template
import alice.library.python.utils as utils
import alice.megamind.library.config.scenario_protos.rpc_handler_config_pb2 as rpc_handler_proto

from .rpc_handler import RpcHandler


def _load_config(filename):
    return utils.load_file(filename, rpc_handler_proto.TRpcHandlerConfigProto())


def _load_graphs(path):
    logging.info(f'Loading ALICE rpc handler graphs: {path}')
    graph_names = []
    for _file in utils.iter_files(path):
        if not _file.name.startswith('rpc_handler_'):
            continue
        graph_names.append(_file.stem)
    return graph_names


def _load_configs(path):
    logging.info(f'Loading Megamind rpc handler configs: {path}')
    rpc_handlers = []
    for _file in utils.iter_files(path, 'common/rpc_handlers'):
        config = _load_config(_file)
        rpc_handlers.append(RpcHandler(config))
    return rpc_handlers


def generate(config_path, graph_path):
    rpc_handlers = _load_configs(config_path)
    rpc_handlers_graphs = _load_graphs(graph_path)
    for c in rpc_handlers:
        subgraph = 'rpc_handler_' + utils.to_snake_case(c.HandlerName)
        if subgraph not in rpc_handlers_graphs:
            raise Exception(f'Cannot generate rpc graph: no {subgraph} subgraph found')

    file_name, config = template.render_mm_rpc_handlers_graph(rpc_handlers)
    utils.write_file(Path(graph_path, file_name), config, to_json=True)
