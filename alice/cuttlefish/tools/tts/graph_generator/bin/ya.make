PY3_PROGRAM(graph-generator)

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/tools/tts/graph_generator/library
)

PY_SRCS(__main__.py)

END()
