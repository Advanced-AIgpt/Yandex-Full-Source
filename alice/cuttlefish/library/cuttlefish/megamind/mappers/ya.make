LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    blackbox_user_info_mapper.cpp
)

PEERDIR(
    alice/library/blackbox/proto
    alice/megamind/protos/speechkit
)

END()
