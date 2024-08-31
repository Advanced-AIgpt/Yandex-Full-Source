PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-6
    infra/yp_service_discovery/python/resolver
    alice/cuttlefish/library/python/mockingbird
    alice/cuttlefish/library/python/test_utils
)

PY_SRCS(
    utils/__init__.py
    __init__.py
    apphost_utils.py
    environment.py
    agent_wrap.py
    apphost_daemon.py
    backend_patcher.py
    horizon_data.py
)

END()
