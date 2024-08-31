PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/apphost_here
    alice/cuttlefish/library/python/uniproxy2_daemon
    alice/cuttlefish/library/python/uniproxy_mock
    alice/cuttlefish/tests/apphost_heavy/mocks
    contrib/python/pytest-asyncio
)

DEPENDS(
    apphost/daemons/app_host
    apphost/tools/event_log_dump
    alice/cuttlefish/bin/cuttlefish
    voicetech/tools/evlogdump
    voicetech/uniproxy2
)

DATA(
    arcadia/alice/cuttlefish/tests/apphost/horizon-data
    arcadia/alice/uniproxy/configs/prod/configs
)

TEST_SRCS(
    utils/tts.py
    __init__.py
    common.py
    conftest.py
    daemons.py
    test_synchronize_state.py
    test_log_spotter.py
    test_vins_text_input.py
)

SIZE(MEDIUM)

REQUIREMENTS(ram:32)

END()
