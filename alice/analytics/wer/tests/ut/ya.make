PY3TEST()

OWNER(nstbezz)

RESOURCE(
    voicetech/asr/tools/asr_analyzer/examples/cluster_references.json /cr.json
    alice/analytics/wer/stop_words.txt /stop.txt
    alice/analytics/wer/sense_words.txt /sense.txt
)

TEST_SRCS(
    test_calculate_metrics.py
)

PEERDIR(
    alice/analytics/wer
)

END()
