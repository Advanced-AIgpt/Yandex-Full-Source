OWNER(g:hollywood)

PY3_LIBRARY()

PEERDIR(
    alice/library/python/template
    contrib/python/Jinja2
)

PY_SRCS(
    __init__.py
)

RESOURCE_FILES(PREFIX graph_
    templates/_custom_alerts.json
    templates/custom_alerts_base.json
    templates/megamind_combinators_{{stage}}.json
    templates/megamind_scenarios_{{stage}}_stage.json
    templates/rpc_megamind.json
    templates/{{scenario_name}}_run.json
    templates/scenario_inputs.json
    templates/deprecated/{{scenario_name}}_run.json
)

END()
