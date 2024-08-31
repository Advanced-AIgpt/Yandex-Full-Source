PROGRAM(megamind_server)

OWNER(g:megamind)

ALLOCATOR(B)

PEERDIR(
    alice/library/geo
    alice/library/logger
    alice/library/metrics
    alice/library/typed_frame
    alice/library/util
    alice/megamind/library/classifiers/formulas
    alice/megamind/library/config
    alice/megamind/library/dispatcher
    alice/megamind/library/globalctx/impl
    alice/megamind/library/handlers/registry
    alice/megamind/library/registry
    alice/megamind/library/scenarios/config_registry
    alice/megamind/library/watchdog
    alice/megamind/protos/common/required_messages
    infra/udp_click_metrics/client
    kernel/factor_slices
    kernel/formula_storage
    library/cpp/getopt
    library/cpp/logger
    library/cpp/monlib/service
    library/cpp/monlib/service/pages
    library/cpp/resource
    library/cpp/sighandler
    apphost/api/service/cpp
    library/cpp/deprecated/atomic
)

SRCS(
    main.cpp
    watchdog.cpp
)

END()
