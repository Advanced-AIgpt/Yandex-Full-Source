PY3_PROGRAM()

OWNER(nstbezz)

PY_SRCS(MAIN main.py)

PEERDIR(
    alice/analytics/wer/lib
)

DEPENDS(
    devtools/experimental/umbrella/embedded/programs/g2p
    voicetech/asr/kaldi_g2p_lingwares/ru
    dict/mt/g2p/transcriber/tool
    dict/mt/tools/token
)

END()
