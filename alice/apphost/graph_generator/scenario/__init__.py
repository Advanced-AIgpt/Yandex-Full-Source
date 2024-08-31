import json
import logging
from pathlib import Path
from collections import defaultdict

import alice.apphost.graph_generator.template as template
import alice.library.python.utils as utils
import alice.megamind.library.config.protos.config_pb2 as config_proto
import alice.megamind.library.config.scenario_protos.config_pb2 as scenario_proto

from .scenario import Scenario


def load_megaming_config(filename):
    return utils.load_file(filename, config_proto.TConfig())


def load_scenario_config(filename):
    return utils.load_file(filename, scenario_proto.TScenarioConfig())


def _load_graphs(path):
    logging.info(f'Loading ALICE graphs: {path}')
    graphs = {}
    for _file in utils.iter_files(path):
        if _file.name.startswith(('combinator', 'megamind', 'modifiers', 'music_sdk', 'polyglot_modifier', 'scenario_inputs')):
            continue
        graph_name = _file.stem
        if graph_name.endswith(template.stages):
            graphs[graph_name] = utils.load_file(_file)
    return graphs


def _load_configs(path):
    logging.info(f'Loading Megamind scenario configs: {path}')
    configs = []
    megamind_config = load_megaming_config(Path(path, 'production/megamind.pb.txt'))
    for _file in utils.iter_files(path, 'dev/scenarios'):
        if _file.name.startswith(('AutoCallsDev', 'Lavka', 'MMGO', 'TvInputs', 'SongTranslation')):
            continue
        config = load_scenario_config(_file)
        configs.append(Scenario(config, megamind_config))
    return configs


none_graphs = [
    'DriveOrder',
    'PhoneNotification',
]


def map_graph_to_stage(graphs):
    graph_stages = defaultdict(list)
    for graph_name in graphs:
        graph_prefix, stage = graph_name.rsplit('_', 1)
        graph_stages[graph_prefix].append(stage)
    return graph_stages


def generate(config_path, graph_path, force_if=lambda _: False, use_deprecated=False):
    graphs = _load_graphs(graph_path)
    scenario_configs = _load_configs(config_path)

    for scenario in scenario_configs:
        if not scenario.is_pure_or_transferring or scenario.Name in none_graphs:
            continue
        graph_name = template.render_scenario_graph_name(scenario)
        graph = graphs.get(graph_name)
        if not graph or force_if(scenario.graph_prefix):
            filename, config = template.render_scenario_graph(scenario, graph, use_deprecated)
            utils.write_file(Path(graph_path, filename), config, to_json=True)
            graphs[graph_name] = json.loads(config)

    custom_alerts_path = Path(graph_path, '_custom_alerts.json')
    custom_alerts = utils.load_file(custom_alerts_path)
    _, config = template.render_custom_alerts(graphs, custom_alerts)
    utils.write_file(custom_alerts_path, config, to_json=True)

    graph_stages = map_graph_to_stage(graphs)
    for scenario in scenario_configs:
        scenario.stages.extend(graph_stages[scenario.graph_prefix])
    for name in template.stages:
        stage_configs = [_ for _ in scenario_configs if name in _.stages or not _.is_pure]
        filename, config = template.render_scenarios_graph(stage_configs, name)
        utils.write_file(Path(graph_path, filename), config, to_json=True)
    scenario_inputs_filename, config = template.render_scenario_inputs_graph(scenario_configs)
    utils.write_file(Path(graph_path, scenario_inputs_filename), config, to_json=True)
