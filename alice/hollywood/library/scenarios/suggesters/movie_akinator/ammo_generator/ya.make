PY3_PROGRAM()

OWNER(
    dan-anastasev
)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
    alice/hollywood/library/scenarios/suggesters/movie_akinator/proto
    alice/megamind/library/session/protos
    alice/megamind/protos/scenarios
)

PY_SRCS(
    MAIN main.py
)

END()
