from .common import SCENARIOS_RUN_GRAPH_NAME, SCENARIOS_APPLY_GRAPH_NAME,\
    SCENARIOS_COMMIT_GRAPH_NAME, SCENARIOS_CONTINUE_GRAPH_NAME, COMINATORS_RUN_GRAPH_NAME, INPUT_NODES, DATA_SOURCES

import alice.megamind.library.config.protos.config_pb2 as config_proto
import google.protobuf.text_format as text
from urllib.parse import urlparse

from alice.megamind.library.config.scenario_protos.combinator_config_pb2 import TCombinatorConfigProto
from alice.megamind.library.config.scenario_protos.config_pb2 import ERequestType, TScenarioConfig

import errno
import json
import os
import re

from collections import defaultdict


def _mkdirs(path):
    if not os.path.exists(path):
        try:
            os.makedirs(path)
        except OSError as err:
            if err.errno != errno.EEXIST:
                raise


def get_local_path(arcadia_root_path, path):
    return os.path.join(arcadia_root_path, path)


def is_app_host_pure_scenario(scenario_config):
    return scenario_config.Handlers.RequestType == ERequestType.AppHostPure


def is_scenario_transferring_to_app_host_pure(scenario_config):
    return scenario_config.Handlers.IsTransferringToAppHostPure


def get_proxy_node_name(config, stage):
    return "SCENARIO_" + config.Name.upper() + "_" + stage.upper()


def get_pure_node_name(config, stage):
    name = config.Name.upper()
    if is_scenario_transferring_to_app_host_pure(config):
        name += "_APP_HOST_COPY"
    return "SCENARIO_" + name + "_" + stage.upper()


def get_pure_request_item_name(scenario_name, stage):
    return "scenario_" + scenario_name + "_" + stage + "_pure_request"


def get_proxy_request_item_name(scenario_name, stage):
    return "scenario_" + scenario_name + "_" + stage + "_http_proxy_request"


def get_pure_response_item_name(scenario_name, stage):
    return "scenario_" + scenario_name + "_" + stage + "_pure_response"


def get_proxy_response_item_name(scenario_name, stage):
    return "scenario_" + scenario_name + "_" + stage + "_http_proxy_response"


def check_graph_existance(graph_name, arcadia_root_path):
    return graph_name + ".json" in os.listdir(get_local_path(arcadia_root_path, "apphost/conf/verticals/ALICE"))


def get_handler(config, arcadia_root_path, stage):
    if is_scenario_transferring_to_app_host_pure(config):
        subgraph_name = config.Handlers.GraphsPrefix + "_" + stage
        if not check_graph_existance(subgraph_name, arcadia_root_path):
            return None
        return config.Handlers.GraphsPrefix + "_" + stage

    conf = json.load(open(get_local_path(arcadia_root_path, "apphost/conf/verticals/ALICE/_routing_config.json")))
    path = urlparse(config.Handlers.BaseUrl).path
    if path[-1] != "/":
        path += "/"
    path += stage
    if path not in conf["paths"]:
        return None
    return conf["paths"][path]["graph"]


def get_timeout_by_field(scenario_name, arcadia_root_path, stage, field_name, field_getter):
    in_str = open(get_local_path(arcadia_root_path, "alice/megamind/configs/production/megamind.pb.txt")).read()
    conf = config_proto.TConfig()
    text.Parse(in_str, conf)

    config_getter = {
        "apply": lambda config, scenario_name: config.HandlersConfig.Apply,
        "commit": lambda config, scenario_name: config.HandlersConfig.Commit,
        "continue": lambda config, scenario_name: config.HandlersConfig.Continue,
        "run": lambda config, scenario_name: config.HandlersConfig.Run
    }[stage]

    default_config = config_getter(conf.Scenarios.DefaultConfig, scenario_name)
    scenario_config = conf.Scenarios.Configs.get(scenario_name)

    if scenario_config is not None:
        scenario_source_config = config_getter(scenario_config, scenario_name)
        if scenario_source_config.HasField(field_name):
            return str(field_getter(scenario_source_config)) + "ms"

    if default_config.HasField(field_name):
        return str(field_getter(default_config)) + "ms"
    return None


def get_timeout(scenario_name, arcadia_root_path, stage):
    def _field_getter(config):
        return config.TimeoutMs
    return get_timeout_by_field(scenario_name, arcadia_root_path, stage, "TimeoutMs", _field_getter)


def get_soft_timeout(scenario_name, arcadia_root_path, stage):
    def _field_getter(config):
        return config.RetryPeriodMs
    return get_timeout_by_field(scenario_name, arcadia_root_path, stage, "RetryPeriodMs", _field_getter)


def make_responsibles(config):
    responsibles = defaultdict(list)
    if len(config.Responsibles.Logins) > 0:
        responsibles["logins"] = list(config.Responsibles.Logins)

    for abc in config.Responsibles.AbcServices:
        resp_abc = {"slug": abc.Name}
        if len(abc.Scopes) != 0:
            resp_abc["role_scopes"] = list(abc.Scopes)
        if len(abc.DutySlugs) != 0:
            resp_abc["duty_slugs"] = list(abc.DutySlugs)
        responsibles["abc_service"].append(resp_abc)
    responsibles["messenger_chat_names"] = ["MegamindDuty"]
    return responsibles


def is_web_search_datasource(data_source):
    return DATA_SOURCES[data_source.Type].startswith("WEB_SEARCH_")


def add_app_host_pure_scenario_to_graph(graph, scenario_config, arcadia_root_path, stage):
    sub_graph_name = get_handler(scenario_config, arcadia_root_path, stage)
    if sub_graph_name is None:
        if stage == "run":
            raise Exception("Can not find suitable graphs for " + scenario_config.Name + " scenario")
        # We do not need apply/commit/run node, if scenario doesn't have corresponding graph
        return graph

    scenario_name = scenario_config.Name
    node_name = get_pure_node_name(scenario_config, stage)

    item_name = get_pure_request_item_name(scenario_name, stage)
    request_meta_item_name = "scenario_" + scenario_name + "_request_meta->mm_scenario_request_meta"
    graph["settings"]["edge_expressions"][INPUT_NODES[stage] + "->" + node_name] = INPUT_NODES[stage] + "[" + item_name + "]"
    graph["settings"]["node_deps"][node_name] = {"input_deps": [
        INPUT_NODES[stage] + "@!^" + request_meta_item_name + ",!^" + item_name + "->mm_scenario_request"
    ]}

    if stage == "run":
        datasources = []
        web_search_datasources = []
        for ds in scenario_config.DataSources:
            data_source_name = ""
            if ds.IsRequired:
                data_source_name += "!"
            data_source_name += "^datasource_" + DATA_SOURCES[ds.Type]
            if is_web_search_datasource(ds):
                web_search_datasources.append(data_source_name)
            else:
                datasources.append(data_source_name)

        if len(datasources) != 0:
            graph["settings"]["node_deps"][node_name]["input_deps"].append("DATASOURCES@" + ",".join(sorted(datasources)))
            graph["settings"]["edge_expressions"][f"DATASOURCES->{node_name}"] = f"{INPUT_NODES[stage]}[{item_name}]"

        if len(web_search_datasources) != 0:
            graph["settings"]["node_deps"][node_name]["input_deps"].append("WEB_SEARCH_DATASOURCES->DATASOURCES@" + ",".join(sorted(web_search_datasources)))
            graph["settings"]["edge_expressions"]["WEB_SEARCH_DATASOURCES->" + node_name] = f"{INPUT_NODES[stage]}[{item_name}]"

    graph["settings"]["nodes"][node_name] = {
        "backend_name": "SELF",
        "node_type": "DEFAULT",
        "params": {
            "attempts": {
                "max_attempts": 1
            },
            "handler": "/_subhost/" + sub_graph_name,
            "responsibles": make_responsibles(scenario_config),
            "timeout": get_timeout(scenario_name, arcadia_root_path, stage)
        }
    }
    graph["settings"]["node_deps"]["RESPONSE"]["input_deps"].append(node_name + "@mm_scenario_response->" + get_pure_response_item_name(scenario_name, stage))
    return graph


def add_app_host_proxy_scenario_to_graph(graph, scenario_config, arcadia_root_path, stage):
    scenario_name = scenario_config.Name
    node_name = get_proxy_node_name(scenario_config, stage)

    item_name = get_proxy_request_item_name(scenario_name, stage)

    graph["settings"]["edge_expressions"][INPUT_NODES[stage] + "->" + node_name] = INPUT_NODES[stage] + "[" + item_name + "]"
    graph["settings"]["node_deps"][node_name] = {"input_deps": [INPUT_NODES[stage] + "@!^" + item_name + "->http_request"]}

    graph["settings"]["nodes"][node_name] = {
        "backend_name": "ALICE__SCENARIO_" + scenario_name.upper() + "_PROXY",
        "node_type": "DEFAULT",
        "params": {
            "attempts": {
                "max_attempts": 1
            },
            "responsibles": make_responsibles(scenario_config),
            "timeout": get_timeout(scenario_name, arcadia_root_path, stage)
        }
    }

    graph["settings"]["nodes"][node_name]["alias_config"] = {"addr_alias": [scenario_name]}

    soft_timeout = get_soft_timeout(scenario_name, arcadia_root_path, stage)
    if soft_timeout is not None:
        graph["settings"]["nodes"][node_name]["params"]["attempts"]["max_attempts"] = 2
        graph["settings"]["nodes"][node_name]["params"]["soft_timeout"] = soft_timeout

    graph["settings"]["node_deps"]["RESPONSE"]["input_deps"].append(node_name + "@http_response->" + get_proxy_response_item_name(scenario_name, stage))
    return graph


def add_scenario_to_graph(graph, scenario_config, arcadia_root_path, stage):
    if is_scenario_transferring_to_app_host_pure(scenario_config):
        graph = add_app_host_pure_scenario_to_graph(graph, scenario_config, arcadia_root_path, stage)
    elif is_app_host_pure_scenario(scenario_config):
        return add_app_host_pure_scenario_to_graph(graph, scenario_config, arcadia_root_path, stage)

    return add_app_host_proxy_scenario_to_graph(graph, scenario_config, arcadia_root_path, stage)


def get_base_graph_dict(stage):
    graph = {
        "monitoring": {
            "alerts": [
                {
                    "crit": 0.1,
                    "operation": "perc",
                    "prior": 10,
                    "type": "failures",
                    "warn": 0.09
                }
            ]
        },
        "settings": {
            "allow_empty_response": True,
            "edge_expressions": {},
            "input_deps": ["SKR", INPUT_NODES[stage]],
            "node_deps": {"RESPONSE": {"input_deps": []}},
            "nodes": {},
            "output_deps": ["RESPONSE"],
            "responsibles": {
                "logins": [
                    "akhruslan",
                    "petrk"
                ]
            },
            "streaming_no_block_outputs": True
        }
    }
    if stage == "run":
        graph["settings"]["input_deps"].extend(["DATASOURCES", "WEB_SEARCH_DATASOURCES"])
    return graph


def get_scenario_configs(arcadia_root_path, mm_configs_dir):
    configs = []
    for file in sorted(os.listdir(get_local_path(arcadia_root_path, os.path.join(mm_configs_dir, "dev", "scenarios")))):
        if file == "ya.make":
            continue
        in_str = open(get_local_path(arcadia_root_path, os.path.join(mm_configs_dir, "dev", "scenarios", file))).read()
        scenario_config = TScenarioConfig()
        text.Parse(in_str, scenario_config)
        if scenario_config.Name in ["AutoCallsDev", "Lavka", "MMGO", "TvInputs", "SongTranslation"]:
            continue
        configs.append(scenario_config)
    return configs


def write_mm_scenarios_graph(output_dir, graph_name, graph):
    graph["settings"]["node_deps"]["RESPONSE"]["input_deps"].sort()
    with open(os.path.join(output_dir, graph_name), "w") as fout:
        json.dump(graph, fout, indent=4, sort_keys=True)
        fout.write("\n")
    print(f"""Created graph {os.path.join(output_dir, graph_name)} with {len(graph["settings"]["nodes"])} scenarios""")


def create_mm_scenarios_graph(mm_configs_dir, app_host_graphs_dir, arcadia_root_path):
    run_graph = get_base_graph_dict("run")
    apply_graph = get_base_graph_dict("apply")
    commit_graph = get_base_graph_dict("commit")
    continue_graph = get_base_graph_dict("continue")

    scenario_configs = get_scenario_configs(arcadia_root_path, mm_configs_dir)
    for scenario_config in scenario_configs:
        run_graph = add_scenario_to_graph(run_graph, scenario_config, arcadia_root_path, "run")
        apply_graph = add_scenario_to_graph(apply_graph, scenario_config, arcadia_root_path, "apply")
        commit_graph = add_scenario_to_graph(commit_graph, scenario_config, arcadia_root_path, "commit")
        continue_graph = add_scenario_to_graph(continue_graph, scenario_config, arcadia_root_path, "continue")

    output_dir = get_local_path(arcadia_root_path, app_host_graphs_dir)
    _mkdirs(output_dir)
    write_mm_scenarios_graph(output_dir, SCENARIOS_RUN_GRAPH_NAME, run_graph)
    write_mm_scenarios_graph(output_dir, SCENARIOS_APPLY_GRAPH_NAME, apply_graph)
    write_mm_scenarios_graph(output_dir, SCENARIOS_COMMIT_GRAPH_NAME, commit_graph)
    write_mm_scenarios_graph(output_dir, SCENARIOS_CONTINUE_GRAPH_NAME, continue_graph)


def get_base_combinators_graph():
    return {
        "monitoring": {
            "alerts": [
                {
                    "crit": 0.1,
                    "operation": "perc",
                    "prior": 10,
                    "type": "failures",
                    "warn": 0.09
                }
            ]
        },
        "settings": {
            "allow_empty_response": True,
            "input_deps": [
                "COMBINATORS_SETUP",
                "SCENARIOS_RUN_STAGE"
            ],
            "node_deps": {
                "RESPONSE": {
                    "input_deps": []
                }
            },
            "nodes": {},
            "output_deps": [
                "RESPONSE"
            ],
            "responsibles": {
                "logins": [
                    "nkodosov"
                ]
            },
            "streaming_no_block_outputs": True
        }
    }


def make_snake_case_name(name):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', name).lower()


def add_combinator_to_graph(graph, combinator_config, arcadia_root_path, scenario_configs_map):
    combinator_node_name = "COMBINATOR_" + make_snake_case_name(combinator_config.Name).upper() + "_RUN"
    combinator_request_item_name = "combinator_request_apphost_type_" + combinator_config.Name

    graph["settings"]["node_deps"]["RESPONSE"]["input_deps"].append(
        combinator_node_name + "@combinator_response_apphost_type->combinator_response_apphost_type_" + combinator_config.Name)

    graph["settings"]["node_deps"][combinator_node_name] = {}
    graph["settings"]["node_deps"][combinator_node_name]["input_deps"] = []
    combinator_setup_dep = "!COMBINATORS_SETUP@!^" + combinator_request_item_name + "->combinator_request_apphost_type"
    for items_dep in combinator_config.Dependences:
        if items_dep.NodeName == "COMBINATORS_SETUP":
            for item_dep in items_dep.Items:
                combinator_setup_dep += "," + ("!" if item_dep.IsRequired else "") + "^" + item_dep.ItemName
            continue

        new_dep = items_dep.NodeName + "@"
        for item_dep in items_dep.Items:
            new_dep += ("!" if item_dep.IsRequired else "") + "^" + item_dep.ItemName + ","

        graph["settings"]["node_deps"][combinator_node_name]["input_deps"].append(new_dep[:-1])
        if items_dep.NodeName not in graph["settings"]["input_deps"]:
            graph["settings"]["input_deps"].append(items_dep.NodeName)

    graph["settings"]["node_deps"][combinator_node_name]["input_deps"].append(combinator_setup_dep)

    if combinator_config.AcceptsAllScenarios:
        graph["settings"]["node_deps"][combinator_node_name]["input_deps"].append(
            "SCENARIOS_RUN_STAGE->INPUT_SCENARIOS_RUN")
    elif len(combinator_config.AcceptedScenarios) > 0:
        dep = "SCENARIOS_RUN_STAGE->INPUT_SCENARIOS_RUN@"
        for acceptedScenario in combinator_config.AcceptedScenarios:
            if acceptedScenario not in scenario_configs_map:
                continue
            scenario_config = scenario_configs_map[acceptedScenario]
            if is_scenario_transferring_to_app_host_pure(scenario_config):
                dep += "^" + get_pure_response_item_name(acceptedScenario, "run") + ","
            elif is_app_host_pure_scenario(scenario_config):
                dep += "^" + get_pure_response_item_name(acceptedScenario, "run") + ","
                continue
            dep += "^" + get_proxy_response_item_name(acceptedScenario, "run") + ","
        graph["settings"]["node_deps"][combinator_node_name]["input_deps"].append(dep[:-1])

    scenario_subgraph_name = "combinator_" + make_snake_case_name(combinator_config.Name) + "_run"

    prod_graphs_dir = os.path.join(arcadia_root_path, "apphost/conf/verticals/ALICE/")
    if not os.path.exists(os.path.join(prod_graphs_dir, scenario_subgraph_name) + ".json"):
        raise Exception("Can not generate graph node for scenario " + combinator_config.Name + ": no scenario subgraph found")

    graph["settings"]["nodes"][combinator_node_name] = {
        "backend_name": "SELF",
        "node_type": "DEFAULT",
        "params": {
            "handler": "/_subhost/" + scenario_subgraph_name,
            "attempts": {
                "max_attempts": 1
            },
            "responsibles": make_responsibles(combinator_config),
            "timeout": "225ms"
        }
    }


def create_mm_combinators_graph(mm_configs_dir, app_host_graphs_dir, arcadia_root_path):
    scenario_configs = get_scenario_configs(arcadia_root_path, mm_configs_dir)
    scenario_configs_map = {}
    for config in scenario_configs:
        scenario_configs_map[config.Name] = config

    graph = get_base_combinators_graph()
    for file in sorted(os.listdir(get_local_path(arcadia_root_path, os.path.join(mm_configs_dir, "dev", "combinators")))):
        if file == "ya.make":
            continue
        in_str = open(get_local_path(arcadia_root_path, os.path.join(mm_configs_dir, "dev", "combinators", file))).read()
        combinator_config = TCombinatorConfigProto()
        text.Parse(in_str, combinator_config)
        add_combinator_to_graph(graph, combinator_config, arcadia_root_path, scenario_configs_map)

    output_dir = get_local_path(arcadia_root_path, app_host_graphs_dir)
    _mkdirs(output_dir)
    with open(os.path.join(output_dir, COMINATORS_RUN_GRAPH_NAME), "w") as fout:
        json.dump(graph, fout, indent=4, sort_keys=True)
        fout.write("\n")

    print(f"""Created graph {os.path.join(output_dir, COMINATORS_RUN_GRAPH_NAME)} with {len(graph["settings"]["nodes"])} combinators""")
