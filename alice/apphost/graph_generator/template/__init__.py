import jinja2
import alice.library.python.template as template


_template = template.Renderer('graph_templates')
_input_nodes = {
    'run': 'WALKER_RUN_STAGE0',
    'apply': 'WALKER_APPLY_PREPARE',
    'commit': 'WALKER_APPLY_PREPARE',
    'continue': 'INPUT_CONTINUE_ITEMS',
}
stages = tuple(_input_nodes.keys())


def render_scenario_graph_name(scenario_config):
    return jinja2.Template('{{scenario_name}}_run').render(
        scenario_name=scenario_config.graph_prefix,
    )


def render_scenario_graph(scenario_config, graph, deprecated=False):
    return _template.render(
        '{{scenario_name}}_run.json',
        deprecated=deprecated,
        scenario=scenario_config,
        graph=graph,
        scenario_name=scenario_config.graph_prefix,
    )


def render_custom_alerts(graphs, custom_alerts):
    return _template.render(
        '_custom_alerts.json',
        graphs=graphs,
        custom_alerts=custom_alerts,
    )


def render_scenarios_graph(graphs, stage):
    return _template.render(
        'megamind_scenarios_{{stage}}_stage.json',
        graphs=graphs,
        stage=stage,
        input_nodes=_input_nodes,
    )


def render_combinators_graph(graphs, stage):
    return _template.render(
        'megamind_combinators_{{stage}}.json',
        graphs=graphs,
        stage=stage,
    )


def render_scenario_inputs_graph(scenarios):
    return _template.render(
        'scenario_inputs.json',
        scenarios=scenarios,
    )


def render_mm_rpc_handlers_graph(graphs):
    return _template.render(
        'rpc_megamind.json',
        graphs=graphs
    )
