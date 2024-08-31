PY3TEST()
SIZE(MEDIUM)

OWNER(
    akhruslan
    petrk
    g:bass
)

PEERDIR(
    alice/megamind/library/config/protos
    alice/megamind/library/config/scenario_protos
)

TEST_SRCS(
    test_configs.py
)

DATA(
    arcadia/alice/megamind/configs
)

END()
