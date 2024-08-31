PY3TEST()

OWNER(
    g:hollywood
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)
PEERDIR(
    alice/hollywood/library/scenarios/vins/proto
    alice/vins/api/vins_api/speechkit/connectors/protocol/protos
)

TEST_SRCS(
    conftest.py
    test_route.py
    test_stages.py
    test_traffic.py
)

REQUIREMENTS(ram:32)

END()
