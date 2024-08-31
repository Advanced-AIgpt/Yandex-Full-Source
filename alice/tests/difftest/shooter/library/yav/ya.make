LIBRARY()

OWNER(sparkle)

PEERDIR(
    alice/joker/library/log
    infra/yp_service_discovery/resolver
    library/cpp/neh
)

SRCS(
    yav.cpp
    yav_requester.cpp
)

END()

RECURSE_FOR_TESTS(ut)
