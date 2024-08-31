UNION()

OWNER(g:hollywood)

IF (SCENARIO_ALARM OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/alarm)
ENDIF()

IF (SCENARIO_DRAW_PICTURE OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/draw_picture)
ENDIF()

IF (SCENARIO_GAME_SUGGEST OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/game_suggest)
ENDIF()

IF (SCENARIO_GENERAL_CONVERSATION OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/general_conversation)
ENDIF()

IF (SCENARIO_IMAGE_WHAT_IS_THIS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/image_what_is_this)
ENDIF()

IF (SCENARIO_MOVIE_AKINATOR OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/movie_akinator)
ENDIF()

IF (SCENARIO_MOVIE_SUGGEST OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/movie_suggest)
ENDIF()

IF (SCENARIO_MUSIC OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/music)
ENDIF()

IF (SCENARIO_NEWS OR NOT DISABLE_ALL_SCENARIOS)
    # TODO: Scenario 'news' has PEERDIR() to search -> need to add search resources to build
    PEERDIR(alice/hollywood/shards/all/prod/resources/search)
ENDIF()

IF (SCENARIO_SEARCH OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/search)
ENDIF()

IF (SCENARIO_SHOW_GIF OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/show_gif)
ENDIF()

IF (SCENARIO_SSSSS OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/sssss)
ENDIF()

IF (SCENARIO_VIDEO_RECOMMENDATION OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(alice/hollywood/shards/all/prod/resources/video_recommendation)
ENDIF()

END()

IF (SCENARIO_ALARM OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(alarm)
ENDIF()

IF (SCENARIO_DRAW_PICTURE OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(draw_picture)
ENDIF()

IF (SCENARIO_GAME_SUGGEST OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(game_suggest)
ENDIF()

IF (SCENARIO_GENERAL_CONVERSATION OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(general_conversation)
ENDIF()

IF (SCENARIO_IMAGE_WHAT_IS_THIS OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(image_what_is_this)
ENDIF()

IF (SCENARIO_MOVIE_AKINATOR OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(movie_akinator)
ENDIF()

IF (SCENARIO_MOVIE_SUGGEST OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(movie_suggest)
ENDIF()

IF (SCENARIO_MUSIC OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(music)
ENDIF()

IF (SCENARIO_NEWS OR NOT DISABLE_ALL_SCENARIOS)
    # note `News` has reference to scenario `Search`
    RECURSE(search)
ENDIF()

IF (SCENARIO_SEARCH OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(search)
ENDIF()

IF (SCENARIO_SHOW_GIF OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(show_gif)
ENDIF()

IF (SCENARIO_SSSSS OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(sssss)
ENDIF()

IF (SCENARIO_VIDEO_RECOMMENDATION OR NOT DISABLE_ALL_SCENARIOS)
    RECURSE(video_recommendation)
ENDIF()
