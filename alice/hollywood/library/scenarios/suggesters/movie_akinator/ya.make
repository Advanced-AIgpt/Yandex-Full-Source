LIBRARY()

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/suggesters/common
    alice/hollywood/library/scenarios/suggesters/movie_akinator/proto
    alice/library/json
    alice/library/proto
    alice/megamind/protos/analytics/scenarios/advisers
    library/cpp/json
)

SRCS(
    movie_base.cpp
    response_body_builder.cpp
)

END()

RECURSE(
    ammo_generator
    content_preparer
)
