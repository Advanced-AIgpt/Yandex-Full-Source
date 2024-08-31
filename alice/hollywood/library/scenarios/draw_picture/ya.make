LIBRARY()

OWNER(
    lvlasenkov
    g:milab
)

PEERDIR(
    alice/hollywood/library/framework
    alice/library/proto

    alice/hollywood/library/scenarios/draw_picture/nlg
    alice/hollywood/library/scenarios/draw_picture/resources/proto
    milab/lib/i2tclient/cpp
)

SRCS(
    draw_picture_impl.cpp
    draw_picture.cpp
    draw_picture_ranked.cpp
    draw_picture_resources.cpp
    GLOBAL register.cpp
)

END()

RECURSE(
    resources/prepare
)

RECURSE_FOR_TESTS(
    it
)
