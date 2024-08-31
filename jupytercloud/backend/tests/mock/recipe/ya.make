PY3_PROGRAM(mock_launcher_recipe)

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/backend/tests/mock/spec

    library/python/testing/recipe
    library/python/testing/yatest_common
    library/recipes/common

    contrib/python/Jinja2
    contrib/python/requests
)

PY_SRCS(
    MAIN main.py
)

RESOURCE_FILES(
    PREFIX jupytercloud/backend/tests/mock/recipe/
    config_template.tpl.py
)

END()

