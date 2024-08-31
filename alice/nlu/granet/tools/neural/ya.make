PY2_PROGRAM(neural_applier)

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/vins/core
    nirvana/valhalla/src
)

PY_SRCS(
    __main__.py
    apply_model.py
    main.py
    utils.py
)

END()
