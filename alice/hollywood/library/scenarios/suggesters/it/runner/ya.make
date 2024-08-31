PY3TEST()

OWNER(
    dan-anastasev
    g:hollywood
    vitvlkv
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/suggesters/games/proto
    alice/hollywood/library/scenarios/suggesters/it
    alice/hollywood/library/scenarios/suggesters/movie_akinator/proto
    alice/hollywood/library/scenarios/suggesters/movies/proto
)

TEST_SRCS(
    test_games.py
    test_movies.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/suggesters/it/data_games
    arcadia/alice/hollywood/library/scenarios/suggesters/it/data_movies
)

REQUIREMENTS(ram:32)

END()
