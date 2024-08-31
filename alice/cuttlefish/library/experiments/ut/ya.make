UNITTEST()
OWNER(g:voicetech-infra)

DATA (
    arcadia/alice/cuttlefish/library/experiments/ut/data
    arcadia/alice/uniproxy/experiments
)

PEERDIR(
    alice/cuttlefish/library/experiments
)

SRCS(
    common.cpp
    test_del.cpp
    test_flags_json.cpp
    test_if_payload.cpp
    test_if_session_data_eq.cpp
    test_if_staff_login.cpp
    test_import_macro.cpp
    test_patch_functions.cpp
    test_sharing_experiments.cpp
    test_smoke.cpp
    test_validation.cpp
)

END()
