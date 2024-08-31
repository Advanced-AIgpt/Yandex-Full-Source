OWNER(
    irinfox
    g:alice_analytics
)

PY23_LIBRARY()

PY_SRCS(
    # currently only adding what we need for marker tests
    __init__.py
    intent_scenario_mapping.py
    usage_fields.py
)

PEERDIR(
    alice/megamind/protos/analytics
    alice/wonderlogs/sdk/python
    contrib/python/protobuf
)

END()

RECURSE_FOR_TESTS(
    ut
    sessions_e2e_tests
)

RECURSE(
    nirvana_runner
    lib_for_py2
    sessions_runner
    videos_nirvana_runner
)
