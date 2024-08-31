PY3TEST()

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/backend/lib/idm

    contrib/python/jsonschema
)

TEST_SRCS(
    test_schema.py
)

END()
