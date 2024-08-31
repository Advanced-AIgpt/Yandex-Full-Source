UNION()

OWNER(
    amullanurov
    elkalinina
    g:hollywood
)

FROM_SANDBOX(FILE 3381424607 RENAME RESOURCE OUT music_targeting.pb)

FROM_SANDBOX(FILE 3381673339 RENAME RESOURCE OUT station_promo.pb)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TMusicShotsFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/music/music_shots.pb.txt
    music_shots.pb
    IN alice/hollywood/shards/common/prod/fast_data/music/music_shots.pb.txt
    OUT music_shots.pb
)


END()
