PY2TEST()

OWNER(g:alice)

SIZE(MEDIUM)

PEERDIR(
    alice/vins/apps/navi
    contrib/python/mock
    contrib/python/requests-mock
)

REQUIREMENTS(network:full)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

DEPENDS(alice/vins/resources)

SRCDIR(alice/vins/apps/navi)

FROM_SANDBOX(FILE 1065285733 OUT_NOAUTO response.data)
FROM_SANDBOX(FILE 1065287025 OUT_NOAUTO response_geo.data)

RESOURCE(response.data response_geosearch_biz.data)
RESOURCE(response_geo.data response_geosearch_geo.data)

TEST_SRCS(
    navi_app/tests/test_geosearch.py
    navi_app/tests/test_navi.py
)

END()
