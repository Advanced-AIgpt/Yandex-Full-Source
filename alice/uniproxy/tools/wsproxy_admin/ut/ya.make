PY3TEST()

OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/aiohttp
    alice/uniproxy/tools/wsproxy_admin
    voicetech/uniproxy2/tests/common
)

DATA(
    arcadia/alice/uniproxy/configs/prod/configs
)

DEPENDS(
    voicetech/uniproxy2
    voicetech/tools/evlogdump
)

TEST_SRCS(
    conftest.py
    test_cgi_parameters.py
    test_host_request.py
    test_perform_requests.py
)

SIZE(SMALL)

END()
