OWNER(g:alice_analytics)

PY23_LIBRARY()

RESOURCE(
    toloka_projects_config.json toloka_projects_config.json
)

END()

RECURSE_FOR_TESTS(
    tests
)
