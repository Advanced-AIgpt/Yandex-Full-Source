PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    actions.py
    actions_application.py
    actions_proxycli.py
    actions_proxy.py
    actions_scenario.py
    actions_report.py
    checks_proxy.py
    context.py
    utils.py
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/aiohttp
    contrib/python/colorama
    library/python/resource
)

RESOURCE(
    application.yaml     /scenarios/application.yaml
)

END()

RECURSE_FOR_TESTS(ut)
