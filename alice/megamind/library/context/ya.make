LIBRARY()

OWNER(g:megamind)

SRCS(
    context.cpp
    fixlist.cpp
    parsed_frames.cpp
    polyglot_translate_utterance_response.cpp
    responses.cpp
    wizard_response.cpp
)

PEERDIR(
    alice/bass/libs/request
    alice/begemot/lib/tagger
    alice/begemot/lib/utils
    alice/library/blackbox/proto
    alice/library/client
    alice/library/experiments
    alice/library/find_poi
    alice/library/frame
    alice/library/geo
    alice/library/json
    alice/library/logger
    alice/library/search
    alice/library/util
    alice/library/request
    alice/library/restriction_level
    alice/library/video_common
    alice/megamind/library/apphost_request/protos
    alice/megamind/library/entity_search
    alice/megamind/library/experiments
    alice/megamind/library/globalctx
    alice/megamind/library/kv_saas
    alice/megamind/library/misspell
    alice/megamind/library/memento
    alice/megamind/library/models/directives
    alice/megamind/library/request
    alice/megamind/library/request/event
    alice/megamind/library/requestctx
    alice/megamind/library/serializers
    alice/megamind/library/search
    alice/megamind/library/session
    alice/megamind/library/sources
    alice/megamind/library/speechkit
    alice/megamind/library/vins
    alice/megamind/library/worldwide/language
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    contrib/libs/protobuf
    library/cpp/geobase
    library/cpp/http/io
    library/cpp/json
    library/cpp/iterator
    library/cpp/scheme
    library/cpp/scheme/util

    search/begemot/rules/alice/parsed_frames/proto
    search/begemot/rules/alice/polyglot_merge_response/proto
    search/begemot/rules/alice/response/proto
    search/begemot/rules/alice/tagger/proto
    search/begemot/rules/occurrences/custom_entities/rule/proto

    search/begemot/apphost
)

END()

RECURSE_FOR_TESTS(ut)
