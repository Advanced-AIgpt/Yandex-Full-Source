UNION()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/shards/general_conversation/prod/resources/game_suggest
    alice/hollywood/shards/general_conversation/prod/resources/general_conversation
    alice/hollywood/shards/general_conversation/prod/resources/movie_suggest
)

END()

RECURSE(
    game_suggest
    general_conversation
    movie_suggest
)
