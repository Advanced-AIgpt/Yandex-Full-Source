PY2TEST()

OWNER(
    ferume-oh3
)


TEST_SRCS(
    test_dialogs_to_sessions.py
)

SIZE(MEDIUM)

DATA(
    # Keep in consistency with https://a.yandex-team.ru/svn/trunk/arcadia/alice/wonderlogs/daily/lib/ut/ya.make?rev=r9634910#L29
    sbr://3303574068=test_make_sessions # input(expboxes, devices, users)
    sbr://3364658567=test_make_sessions # output(sessions)
)

PEERDIR(
    contrib/python/pytest
    statbox/nile
    statbox/nile_debug
    statbox/qb2
    yql/library/python

    alice/analytics/utils/testing_utils
    alice/analytics/operations/dialog/sessions/lib_for_py2
)

END()
