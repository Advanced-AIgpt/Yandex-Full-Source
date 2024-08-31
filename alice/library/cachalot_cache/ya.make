LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    cachalot_cache.cpp
)

PEERDIR(
    alice/cachalot/api/protos
)

END()

RECURSE_FOR_TESTS(ut)
