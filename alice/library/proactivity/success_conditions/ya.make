LIBRARY()

OWNER(
    jan-fazli
    karina-usm
    olegator
)

PEERDIR(
    contrib/libs/re2
    dj/services/alisa_skills/server/proto/data
    library/cpp/json
)

SRCS(
    match_success_conditions.cpp
)

END()

RECURSE_FOR_TESTS(ut)
