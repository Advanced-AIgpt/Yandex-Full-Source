PY3_PROGRAM()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/scenarios/alice_show/proto
    alice/hollywood/library/scenarios/blueprints/proto
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/hollywood/library/scenarios/hardcoded_response/proto
    alice/hollywood/library/scenarios/how_to_spell/proto
    alice/hollywood/library/scenarios/market/common/proto
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/news/proto
    alice/hollywood/library/scenarios/notifications/proto
    alice/hollywood/library/scenarios/random_number/proto
    alice/hollywood/library/scenarios/sssss/proto
    alice/hollywood/protos
    alice/megamind/library/search/protos
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

PY_SRCS(
    __main__.py
)

END()
