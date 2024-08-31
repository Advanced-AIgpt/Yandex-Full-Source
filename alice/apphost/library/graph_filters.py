import os
import re


def leave_only_json_files(files):
    return [file for file in files if file.endswith(".json")]


def leave_only_graph_files(files):
    return [file for file in leave_only_json_files(files) if not os.path.split(file)[1].startswith('_')]


def leave_only_scenario_graphs_files(files):
    graphs = leave_only_graph_files(files)
    result = []
    filename_re = re.compile('(?!combinator_)^(.+)_(run|commit|apply|continue).json$')
    skip_names = {
        'megamind_apply.json',
        'modifiers_run.json',
        'megamind_combinators_run.json',
        'megamind_combinators_continue.json',
        'polyglot_modifier_run.json',

        # music subgraphs
        'music_sdk_run.json',
        'music_sdk_continue.json',
    }
    for file in graphs:
        graph = os.path.split(file)[1]
        if not filename_re.match(graph):
            continue
        if graph in skip_names:
            continue
        result.append(file)
    return result


def leave_only_graphs_with_hollywood_backends(files):
    return [file for file in leave_only_graph_files(files) if '"HOLLYWOOD_ALL"' in open(file).read()]


def leave_only_hollywood_scenarios_files(files):
    scenarios = set(leave_only_scenario_graphs_files(files))
    hollywood = set(leave_only_graphs_with_hollywood_backends(files))
    return list(scenarios.intersection(hollywood))


def leave_only_generated_megamind_scenarios_graphs_files(files):
    return [file for file in files if os.path.split(file)[1].startswith("megamind_scenarios_") and
                                      os.path.split(file)[1].endswith("_stage.json")]
