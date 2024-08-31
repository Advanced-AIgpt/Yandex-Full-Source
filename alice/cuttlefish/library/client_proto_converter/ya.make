LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    convert.cpp
)
PEERDIR(
    contrib/deprecated/jsoncpp
    alice/cuttlefish/library/speechkit_proto
)

END()

RECURSE_FOR_TESTS(ut)
