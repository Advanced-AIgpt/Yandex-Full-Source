# sample from documetantion https://doc.yandex-team.ru/music/api-guide/concepts/service-search_track-recognize.xml
# oauth tonek owned and42@
set -x
curl -v -X POST "https://match.music.yandex.net/match/api/upload-json" \
    -H "Authorization: OAuth AQAAAAAB3TzrAAAgQ5Icgx4M80tEjbGZc1tiWLg" \
    -H "X—Yandex—Music—Client: YandexMusicAndroid/2.14" \
    -H "X—Yandex—Music—Device: os=Android; os_version=4.0.4; manufacturer=samsung; model=GT-S7562; clid=google-play; device_id=e6d929184598f74c9266dcab698d2a4a; uuid=a8eb10ded5380af021aa12b25a9d0a0f" \
    -H "Transfer-Encoding: chunked" \
    -H "Content-Type: audio/pcm-data" \
    --data-binary "@music_record.pcm"
