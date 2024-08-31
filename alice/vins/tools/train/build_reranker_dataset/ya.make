PY2_PROGRAM(build_reranker_dataset)

OWNER(
    dan-anastasev
    g:alice_quality
    g:alice
)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/core
    contrib/python/click
)

PY_SRCS(
    __main__.py
)

END()
