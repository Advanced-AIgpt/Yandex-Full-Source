LIBRARY()

OWNER(g:voicetech-infra)


SRCS(
    aggregation.cpp
    backend.cpp
    dummy.cpp
    engine.cpp
    golovan.cpp
    metrics.cpp
    settings.cpp
    solomon.cpp
    storage.cpp
)

PEERDIR(
    library/cpp/http/misc
    library/cpp/monlib/counters
    library/cpp/monlib/encode
    library/cpp/monlib/encode/json
    library/cpp/monlib/encode/spack
    library/cpp/monlib/metrics
    library/cpp/unistat
)

GENERATE_ENUM_SERIALIZATION(backend.h)

END()

RECURSE_FOR_TESTS(ut)
