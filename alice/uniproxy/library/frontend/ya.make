PY3_LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4
    library/python/resource
    alice/uniproxy/library/common_handlers
    alice/uniproxy/library/frontend/resources
    voicetech/infra/library/tornado_resource_handlers
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(ut)
