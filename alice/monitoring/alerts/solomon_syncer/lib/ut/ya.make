PY3TEST()

OWNER(g:alice_fun)

PEERDIR(
    alice/monitoring/alerts/solomon_syncer/lib
)

TEST_SRCS(
    sync_ut.py
)

END()
