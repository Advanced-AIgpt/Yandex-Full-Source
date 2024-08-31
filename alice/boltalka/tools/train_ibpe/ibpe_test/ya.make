OWNER(
    alipov
    g:alice_boltalka
)

PY2TEST()

TEST_SRCS(
    test.py
)

DEPENDS(
    alice/boltalka/tools/train_ibpe
)

SIZE(SMALL)

DATA(
    arcadia/alice/boltalka/tools/train_ibpe/ibpe_test/data/sentences_data.tsv
    arcadia/alice/boltalka/tools/train_ibpe/ibpe_test/data/char_data.tsv
)

END()
