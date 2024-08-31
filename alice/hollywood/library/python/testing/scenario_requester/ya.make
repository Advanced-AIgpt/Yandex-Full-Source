PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    apphost/python/client
    alice/megamind/protos/scenarios
    library/python/langs
)

PY_SRCS(
    __init__.py
    scenario_requester.py
)

END()
