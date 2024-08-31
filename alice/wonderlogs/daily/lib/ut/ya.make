YT_UNITTEST()

OWNER(g:wonderlogs)

SIZE(medium)

INCLUDE(${ARCADIA_ROOT}/mapreduce/yt/python/recipe/recipe.inc)

SRCS(
    asr_prepared_ut.cpp
    dialogs_ut.cpp
    differ_ut.cpp
    expboxes_ut.cpp
    megamind_prepared_ut.cpp
    uniproxy_prepared_ut.cpp
    wonderlogs_ut.cpp
)

PEERDIR(
    alice/library/json
    alice/library/unittest
    alice/wonderlogs/daily/lib

    mapreduce/yt/tests/yt_unittest_lib
    mapreduce/yt/tests/yt_unittest_main
)

FROM_SANDBOX(
    3374318685
    OUT_NOAUTO
    asr_error.jsonlines
    asr_logs.yson
    asr_prepared.jsonlines
    censored_dialogs1.yson
    censored_dialogs2.yson
    censored_wonderlogs1.jsonlines
    censored_wonderlogs2.jsonlines
    dialogs.yson
    dialogs1.yson
    dialogs2.yson
    dialogs_error.yson
    expboxes.yson
    expboxes_error.yson
    megamind_analytics_logs.yson
    megamind_error.jsonlines
    megamind_prepared.jsonlines
    private_users.jsonlines
    private_wonderlogs.jsonlines
    banned_dialogs.yson
    robot_dialogs.yson
    robot_dialogs_error.yson
    robot_wonderlogs.jsonlines
    uniproxy_error.jsonlines
    uniproxy_events.yson
    uniproxy_prepared.jsonlines
    wonderlogs.jsonlines
    wonderlogs1.jsonlines
    wonderlogs2.jsonlines
    wonderlogs_error.jsonlines
)

DATA(
    sbr://3351884173 # geodata6.bin 2022-07-22
)

RESOURCE(
    asr_error.jsonlines asr_error.jsonlines
    asr_logs.yson asr_logs.yson
    asr_prepared.jsonlines asr_prepared.jsonlines
    censored_dialogs1.yson censored_dialogs1.yson
    censored_dialogs2.yson censored_dialogs2.yson
    censored_wonderlogs1.jsonlines censored_wonderlogs1.jsonlines
    censored_wonderlogs2.jsonlines censored_wonderlogs2.jsonlines
    dialogs.yson dialogs.yson
    dialogs1.yson dialogs1.yson
    dialogs2.yson dialogs2.yson
    dialogs_error.yson dialogs_error.yson
    expboxes.yson expboxes.yson
    expboxes_error.yson expboxes_error.yson
    megamind_analytics_logs.yson megamind_analytics_logs.yson
    megamind_error.jsonlines megamind_error.jsonlines
    megamind_prepared.jsonlines megamind_prepared.jsonlines
    private_users.jsonlines private_users.jsonlines
    private_wonderlogs.jsonlines private_wonderlogs.jsonlines
    banned_dialogs.yson banned_dialogs.yson
    robot_dialogs.yson robot_dialogs.yson
    robot_dialogs_error.yson robot_dialogs_error.yson
    robot_wonderlogs.jsonlines robot_wonderlogs.jsonlines
    uniproxy_error.jsonlines uniproxy_error.jsonlines
    uniproxy_events.yson uniproxy_events.yson
    uniproxy_prepared.jsonlines uniproxy_prepared.jsonlines
    wonderlogs.jsonlines wonderlogs.jsonlines
    wonderlogs1.jsonlines wonderlogs1.jsonlines
    wonderlogs2.jsonlines wonderlogs2.jsonlines
    wonderlogs_error.jsonlines wonderlogs_error.jsonlines
)

REQUIREMENTS(ram:12)

END()
