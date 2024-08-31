PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    checks.py
    config_patch.py
    http_request.py
    mocks/__init__.py
    mocks/async_http_stream.py
    mocks/blackbox_server.py
    mocks/mds_server.py
    mocks/common.py
    mocks/log_collector.py
    mocks/personal_data_helper.py
    mocks/proto_server.py
    mocks/rt_log.py
    mocks/server.py
    mocks/spotter_stream.py
    mocks/tts_stream.py
    mocks/tvm_server.py
    mocks/uaas.py
    mocks/uni_web_socket.py
    mocks/yabio_stream.py
    mocks/yaldi_stream.py
    wrappers/__init__.py
    wrappers/vins.py
    wrappers/vins_request.py
)

PEERDIR(
    alice/uniproxy/library/auth
    alice/uniproxy/library/auth/mocks
    alice/uniproxy/library/backends_common
    alice/uniproxy/library/processors
    alice/uniproxy/library/settings
    alice/uniproxy/library/uaas
    alice/uniproxy/library/unisystem
    alice/uniproxy/library/vins
    contrib/python/tornado/tornado-4
    library/python/pytest
    voicetech/library/proto_api
)

END()
