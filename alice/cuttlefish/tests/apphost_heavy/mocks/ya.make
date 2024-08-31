PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-6
    alice/cachalot/api/protos
    alice/cuttlefish/library/python/apphost_grpc_servant
    alice/cuttlefish/library/python/test_utils
    alice/cuttlefish/library/python/mockingbird
    alice/protos/data/location
)

PY_SRCS(
    __init__.py
    blackbox.py
    cachalot.py
    datasync.py
    laas.py
    mds.py
    megamind.py
    memento.py
    quasar_iot.py
)

END()
