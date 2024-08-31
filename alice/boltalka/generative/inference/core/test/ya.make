UNITTEST_FOR(alice/boltalka/generative/inference/core)

OWNER(
    g:alice_boltalka
    artemkorenev
)

SIZE(MEDIUM)

TAG(ya:yt ya:noretries)
YT_SPEC(dict/mt/gpu_yt_spec.yson)

CFLAGS(-DUSE_GPU)

REQUIREMENTS(ram:32 ram_disk:8)

SRCS(
    boltalka_generation_ut.cpp
    filtering.cpp
    test_tokenizer.cpp
)

PEERDIR(
    library/cpp/threading/future
)

FORK_SUBTESTS()

DATA(
    sbr://1279829320=base_transformer  # ./base_transformer/data/model.npz
    sbr://2200078069=zeliboba_1_0_boltalka  # ./zeliboba_1_0_boltalka/v19_better/model.npz
)

END()
