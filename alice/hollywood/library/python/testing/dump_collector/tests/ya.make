PY3TEST()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/hollywood/library/python/testing/dump_collector
    alice/tests/library/service
)

TEST_SRCS(
    tests.py
)

DEPENDS(apphost/tools/event_log_dump)

FROM_SANDBOX(FILE 2645435999 OUT_NOAUTO eventlog-4269)
RESOURCE(eventlog-4269 eventlog-run_contiue)

FROM_SANDBOX(FILE 2645744860 OUT_NOAUTO eventlog-7427)
RESOURCE(eventlog-7427 eventlog-run)

FROM_SANDBOX(FILE 2645904369 OUT_NOAUTO eventlog-15033)
RESOURCE(eventlog-15033 eventlog-run_apply)

FROM_SANDBOX(FILE 2645927266 OUT_NOAUTO eventlog-15886)
RESOURCE(eventlog-15886 eventlog-run_commit)

FROM_SANDBOX(FILE 2709860676 OUT_NOAUTO eventlog-10605)
RESOURCE(eventlog-10605 eventlog-app_host_copy_run)

END()
