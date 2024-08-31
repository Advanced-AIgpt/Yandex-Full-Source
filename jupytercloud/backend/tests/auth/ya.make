PY3TEST()

OWNER(g:jupyter-cloud)

SIZE(SMALL)

INCLUDE(${ARCADIA_ROOT}/jupytercloud/backend/tests/mock/recipe/recipe.inc)

TEST_SRCS(
    conftest.py
    test_auth.py
)

END()
