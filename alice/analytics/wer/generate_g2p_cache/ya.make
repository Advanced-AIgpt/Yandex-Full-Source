OWNER(g:yatool)

PY3_PROGRAM()
PEERDIR(
    voicetech/asr/tools/metrics/lib
    alice/analytics/wer/lib
)
PY_SRCS(
    MAIN main.py
)

END()
