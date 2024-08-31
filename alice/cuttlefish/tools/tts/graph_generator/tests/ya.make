PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

TEST_SRCS(test.py)

PEERDIR(
    alice/cuttlefish/tools/tts/graph_generator/library
)

DATA(
    # Graphs in apphost dir
    arcadia/apphost/conf/verticals/VOICE
    # All possible tts backend request item types
    arcadia/alice/cuttlefish/library/cuttlefish/tts/utils/tests_canonize/canondata
)

END()
