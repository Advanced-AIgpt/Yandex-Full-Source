PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)


PY_SRCS(
    __init__.py
    registry_base.py
    subway_registry.py
    unisystem_registry.py
)

END()


RECURSE_FOR_TESTS(
    ut
)
