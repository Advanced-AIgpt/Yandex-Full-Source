import logging
from pathlib import Path

import alice.apphost.graph_generator.scenario as scenario
import alice.apphost.graph_generator.template as template
import alice.library.python.utils as utils
import alice.megamind.library.config.scenario_protos.combinator_config_pb2 as combinator_proto

from .combinator import Combinator


def _load_config(filename):
    return utils.load_file(filename, combinator_proto.TCombinatorConfigProto())


def _load_graphs(path):
    logging.info(f'Loading ALICE combinator graphs: {path}')
    graph_names = []
    for _file in utils.iter_files(path):
        if not _file.name.startswith('combinator'):
            continue
        graph_name = _file.stem
        if graph_name.endswith(template.stages):
            graph_names.append(graph_name)
    return graph_names


def _load_configs(path):
    logging.info(f'Loading Megamind combinator configs: {path}')
    combinators = []
    for _file in utils.iter_files(path, 'dev/combinators'):
        config = _load_config(_file)
        combinators.append(Combinator(config))
    return combinators


def generate(config_path, graph_path):
    combinators = _load_configs(config_path)
    combinator_graphs = _load_graphs(graph_path)
    combinator_graph_stages = scenario.map_graph_to_stage(combinator_graphs)
    for name in template.stages:
        stage_configs = [_ for _ in combinators if name in combinator_graph_stages[_.graph_prefix]]
        if stage_configs:
            filename, config = template.render_combinators_graph(stage_configs, name)
            utils.write_file(Path(graph_path, filename), config, to_json=True)
