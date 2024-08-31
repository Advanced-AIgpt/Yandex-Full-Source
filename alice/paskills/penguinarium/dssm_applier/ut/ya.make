PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(MEDIUM)

TEST_SRCS(
    test_dssm_applier.py
)

PEERDIR(
    alice/paskills/penguinarium/dssm_applier
)

DATA(
    # assessor.dssm
    sbr://913610077
)

END()
