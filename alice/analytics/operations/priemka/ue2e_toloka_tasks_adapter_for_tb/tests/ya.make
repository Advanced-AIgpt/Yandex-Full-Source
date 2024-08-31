OWNER(g:alice_analytics)

PY3TEST()

TEST_SRCS(
    test_utils.py
)

PEERDIR(
    contrib/python/pytest

    alice/analytics/operations/priemka/ue2e_toloka_tasks_adapter_for_tb
)

END()
