import alice.megamind.protos.common.data_source_type_pb2 as ds_type


SCENARIOS_RUN_GRAPH_NAME = "megamind_scenarios_run_stage.json"
SCENARIOS_APPLY_GRAPH_NAME = "megamind_scenarios_apply_stage.json"
SCENARIOS_COMMIT_GRAPH_NAME = "megamind_scenarios_commit_stage.json"
SCENARIOS_CONTINUE_GRAPH_NAME = "megamind_scenarios_continue_stage.json"

COMINATORS_RUN_GRAPH_NAME = "megamind_combinators_run.json"

MEGAMIND_SCENARIOS_GRAPHS = [
    SCENARIOS_APPLY_GRAPH_NAME,
    SCENARIOS_COMMIT_GRAPH_NAME,
    SCENARIOS_CONTINUE_GRAPH_NAME,
    SCENARIOS_RUN_GRAPH_NAME
]

INPUT_NODES = {"run": "WALKER_RUN_STAGE0", "apply": "WALKER_APPLY_PREPARE", "commit": "WALKER_APPLY_PREPARE", "continue": "WALKER_POST_CLASSIFY"}

DATA_SOURCES = {}
for v, k in ds_type.EDataSourceType.items():
    DATA_SOURCES[k] = v
