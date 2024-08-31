LIBRARY()

OWNER(
    lvlasenkov
    g:milab
)

PEERDIR(
    alice/hollywood/library/framework
    alice/library/proto
    alice/hollywood/library/scenarios/transform_face/nlg
    alice/hollywood/library/scenarios/transform_face/proto
    extsearch/images/daemons/cbirdaemon2/response_proto
)

SRCS(
    transform_face_impl.cpp
    transform_face_render.cpp
    transform_face_run.cpp
    transform_face_continue.cpp
    GLOBAL register.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
