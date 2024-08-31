UNION()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/shards/common/prod/resources/alarm
    alice/hollywood/shards/common/prod/resources/draw_picture
    alice/hollywood/shards/common/prod/resources/game_suggest
    alice/hollywood/shards/common/prod/resources/image_what_is_this
    alice/hollywood/shards/common/prod/resources/movie_akinator
    alice/hollywood/shards/common/prod/resources/movie_suggest
    alice/hollywood/shards/common/prod/resources/music
    alice/hollywood/shards/common/prod/resources/show_gif
    alice/hollywood/shards/common/prod/resources/sssss
    alice/hollywood/shards/common/prod/resources/video_recommendation
)

END()

RECURSE(
    alarm
    draw_picture
    game_suggest
    image_what_is_this
    movie_akinator
    movie_suggest
    music
    search
    show_gif
    sssss
    video_recommendation
)
