UNITTEST()

OWNER(
    nzinov
    g:alice_boltalka
)

SIZE(MEDIUM)
TAG(ya:yt ya:noretries)
YT_SPEC(alice/boltalka/bert/ut/gpu_yt_spec.yson)

CFLAGS(-DUSE_GPU)
PEERDIR(dict/mt/libs/nn/ynmt/test_utils/gpu)

INCLUDE(${ARCADIA_ROOT}/alice/boltalka/bert/ut/ya.make.inc)

END()
