PY3_LIBRARY()

OWNER(
    g:milab
    valerymal
)

PEERDIR(
    alice/tests/library/surface
    alice/tests/library/scenario
    library/python/pytest
)

PY_SRCS(
    draw_picture.py
    transform_face.py
)

END()
