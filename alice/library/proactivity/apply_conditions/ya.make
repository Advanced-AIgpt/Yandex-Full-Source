LIBRARY()

OWNER(
    ayanginet
    jan-fazli
    karina-usm
    olegator
)

PEERDIR(
    alice/megamind/protos/speechkit
    dj/services/alisa_skills/server/proto/data
)

SRCS(
    check_apply_conditions.cpp
)

END()

RECURSE_FOR_TESTS(ut)
