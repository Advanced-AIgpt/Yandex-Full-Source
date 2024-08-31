PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/apphost/fixture
    alice/bass/fixture
    alice/hollywood/fixture
    alice/hollywood/library/python/testing/stubber
    alice/hollywood/library/python/testing/scenario_requester
    alice/library/python/testing/auth
    alice/megamind/protos/scenarios
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    conftest.py
    test_functions.py
)

END()
