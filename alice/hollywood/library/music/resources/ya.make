# NOTE: This file will be included by a shard that wants the scenario's resources,
# so you must always load files by the absolute path and save them in the current directory.
# You can also make free use of FROM_SANDBOX statements.
#
# You **cannot** use the FILES statement, but it's actually equivalent to several COPY_FILE statements
# so just use those instead.

UNION()

OWNER(g:hollywood)

COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/novelty.json novelty.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/need_similar.json need_similar.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/mood.json mood.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/genre.json genre.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/activity.json activity.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/epoch.json epoch.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/language.json language.json
)
COPY_FILE(
    alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/vocal.json vocal.json
)
COPY_FILE(
    maps/automotive/radio/radio.json radio_station.json
)
COPY_FILE(
    alice/bass/data/radio_stations.json radio_stations.json
)

END()
