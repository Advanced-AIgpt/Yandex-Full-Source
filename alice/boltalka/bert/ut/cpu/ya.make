UNITTEST()

OWNER(
    nzinov
    g:alice_boltalka
)

SIZE(MEDIUM)

PEERDIR(dict/mt/libs/nn/ynmt/test_utils/cpu)

INCLUDE(${ARCADIA_ROOT}/alice/boltalka/bert/ut/ya.make.inc)

END()