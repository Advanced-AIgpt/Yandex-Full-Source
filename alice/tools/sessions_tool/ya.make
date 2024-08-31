PROGRAM()

OWNER(g:megamind)

SRCS(
    main.cpp
    parse_scenario_session.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/library/json
    alice/library/proto
    alice/megamind/library/session
    alice/vins/api/vins_api/speechkit/connectors/protocol/protos
    library/cpp/getopt
    library/cpp/json
)

END()

