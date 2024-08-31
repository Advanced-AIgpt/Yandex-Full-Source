LIBRARY()

OWNER(
    jan-fazli
    g:megamind
)

PEERDIR(
    alice/library/json
    alice/library/network
    alice/megamind/library/classifiers
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/response
    alice/megamind/library/scenarios/helpers/interface
    alice/megamind/library/scenarios/interface
    alice/megamind/library/session
    alice/megamind/library/sources
    alice/megamind/library/speechkit
    alice/megamind/library/util
    alice/megamind/protos/modifiers
    alice/megamind/protos/proactivity
    alice/protos/data/scenario/music
    dj/services/alisa_skills/server/proto/client
    library/cpp/langs
    library/cpp/scheme
)

SRCS(
    common.cpp
)

END()
