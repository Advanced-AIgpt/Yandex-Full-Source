LIBRARY()

OWNER(
    stupnik
    g:hollywood
    g:alice
)

JOIN_SRCS(
    all.cpp
    content_id.cpp
    playlist_id.cpp
    similar_radio_id.cpp
    track_album_id.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/util
)

END()
