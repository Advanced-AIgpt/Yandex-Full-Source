PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/generative/tfnn/bucket_maker/lib
    nirvana/valhalla/src
)

PY_SRCS(
    __init__.py
    ops.py
)

END()
