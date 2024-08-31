LIBRARY()

OWNER(
    g:megamind
    g:alice_quality
)

PEERDIR(
    alice/begemot/lib/api/experiments
    alice/begemot/lib/api/params
    alice/begemot/lib/locale
    alice/library/contacts
    alice/library/experiments
    alice/library/json
    alice/library/network
    alice/library/video_common
    alice/megamind/library/apphost_request
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/context
    alice/megamind/library/search
    alice/megamind/library/sources
    alice/megamind/library/util
    alice/megamind/library/worldwide/language
    alice/megamind/protos/common
    alice/nlu/granet/lib/user_entity
    alice/nlu/libs/request_normalizer

    contrib/libs/protobuf

    library/cpp/json
    library/cpp/string_utils/base64

    search/begemot/apphost
    search/begemot/rules/alice/response/proto
    search/begemot/rules/alice/session/proto
)

SRCS(
    begemot.cpp
)

END()

RECURSE_FOR_TESTS(ut)
