LIBRARY()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos
    library/cpp/json
    library/cpp/regex/pcre
    library/cpp/string_utils/base64
)

SRCS(
    event_patcher.cpp
    event_patcher.h
    experiment_patch.cpp
    experiment_patch.h
    experiments.cpp
    experiments.h
    flags_json.cpp
    local_experiments.cpp
    local_experiments.h
    patch_functions.cpp
    patch_functions.h
    session_context_proxy.cpp
    session_context_proxy.h
    utils.cpp
    utils.h
)

END()

RECURSE_FOR_TESTS(ut)
