OWNER(g:hollywood)

PY3_LIBRARY()

PEERDIR(
    alice/library/python/utils
    contrib/python/Jinja2
)

PY_SRCS(
    __init__.py
    product_scenarios.py
    scenario_config.py
    shards.py
    yamake.py
)

RESOURCE_FILES(PREFIX scenario_
    templates/{{scenario_name}}/it2/tests.py
    templates/{{scenario_name}}/it2/ya.make

    templates/{{scenario_name}}/nlg/ya.make
    templates/{{scenario_name}}/nlg/{{scenario_name}}_ru.nlg

    templates/{{scenario_name}}/proto/ya.make
    templates/{{scenario_name}}/proto/{{scenario_name}}.proto

    templates/{{scenario_name}}/ut/ya.make

    templates/{{scenario_name}}/ya.make
    templates/{{scenario_name}}/{{scenario_name}}.cpp
    templates/{{scenario_name}}/{{scenario_name}}.h
    templates/{{scenario_name}}/{{scenario_name}}_dispatch_ut.cpp
    templates/{{scenario_name}}/{{scenario_name}}_render_ut.cpp
    templates/{{scenario_name}}/{{scenario_name}}_scene.cpp
    templates/{{scenario_name}}/{{scenario_name}}_scene.h
)

END()
