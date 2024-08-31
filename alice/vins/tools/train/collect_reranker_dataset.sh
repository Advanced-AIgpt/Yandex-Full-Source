#!/usr/bin/env bash

# run as: CUDA_VISIBLE_DEVICES=-1 VINS_NUM_PROCS=32 OMP_NUM_THREADS=1 ./tools/train/collect_reranker_dataset.sh \
# <train.tsv path> <train dataset output path> <test.tsv path> <test dataset output path> <output path to folder with reranker dataset in tsv format> [<retries_count>]

collect_train_dataset_cmd="echo 'Collecting train dataset' && \
python tools/train/reranker_dataset_collection.py -i $1 -o $2"
collect_test_dataset_cmd="echo 'Collecting test dataset' && \
python tools/train/reranker_dataset_collection.py -i $3 -o $4"

add_ner_factor_cmd="echo 'Collecting ner features' && \
python tools/train/add_ner_factor.py --data-path $2 --ner-type ner && \
python tools/train/add_ner_factor.py --data-path $4 --ner-type ner"

add_wizard_factor_cmd="echo 'Collecting wizard features' && \
python tools/train/add_ner_factor.py --data-path $2 --ner-type wizard && \
python tools/train/add_ner_factor.py --data-path $4 --ner-type wizard"

add_scenarios_lstm_factor_cmd="echo 'Collecting scenarios lstm features' && \
python tools/train/train_word_nn.py --classifier-name scenarios predict \
--model-path $VINS_RESOURCES_PATH/scenarios_lstm_model/scenarios_lstm_model/ \
$2 $4 --feature-name scenarios_lstm-model"

add_toloka_lstm_factor_cmd="echo 'Collecting toloka lstm features' && \
python tools/train/train_word_nn.py --classifier-name scenarios predict \
--model-path $VINS_RESOURCES_PATH/toloka_lstm_model/toloka_lstm_model/ \
$2 $4 --feature-name toloka_lstm-model --map-to scenarios"

add_device_state_factor_cmd="echo 'Collecting device state features' && \
python tools/train/add_device_state_factor.py --train-data-path $2 --val-data-path $4"

add_prev_intent_factor_cmd="echo 'Collecting prev intent features' && \
python tools/train/add_prev_intent_factor.py --train-data-path $2 --val-data-path $4"

if [ -z "$6" ]; then
    retries_count="--retries-count $6"
else
    retries_count=""
fi

add_entity_factor_cmd="echo 'Collecting entity features' && \
python tools/train/collect_entities.py --data-path $2 $retries_count && \
python tools/train/add_entity_factor.py --data-path $2 && \
python tools/train/collect_entities.py --data-path $4 $retries_count && \
python tools/train/add_entity_factor.py --data-path $4"

build_reranker_dataset_params="--skip-intents personal_assistant.scenarios.player_pause,\
personal_assistant.handcrafted.cancel,\
personal_assistant.scenarios.sound_set_level,\
personal_assistant.scenarios.select_video_from_gallery_by_text,\
personal_assistant.stroka.open_settings,\
personal_assistant.scenarios.quasar.payment_confirmed,\
personal_assistant.stroka.clear_history,\
personal_assistant.scenarios.player_next_track,\
personal_assistant.handcrafted.user_reactions_negative_feedback,\
personal_assistant.scenarios.quasar.go_backward,\
personal_assistant.scenarios.show_traffic,\
personal_assistant.scenarios.radio_play,\
personal_assistant.scenarios.quasar.open_current_video,\
personal_assistant.scenarios.music_sing_song,\
personal_assistant.scenarios.sound_quiter,\
personal_assistant.stroka.open_file,\
personal_assistant.scenarios.onboarding,\
personal_assistant.scenarios.get_weather_nowcast,\
personal_assistant.scenarios.taxi_order,\
personal_assistant.scenarios.how_much,\
personal_assistant.scenarios.alarm_cancel,\
personal_assistant.scenarios.player_continue,\
personal_assistant.scenarios.get_date,\
personal_assistant.scenarios.get_my_location,\
personal_assistant.scenarios.repeat,\
personal_assistant.stroka.go_home,\
personal_assistant.scenarios.call,\
personal_assistant.scenarios.timer_set,\
personal_assistant.scenarios.image_what_is_this,\
personal_assistant.scenarios.games_onboarding,\
personal_assistant.scenarios.player_previous_track,\
personal_assistant.handcrafted.user_reactions_positive_feedback,\
personal_assistant.scenarios.player_rewind,\
personal_assistant.scenarios.player_dislike,\
personal_assistant.scenarios.get_news,\
personal_assistant.scenarios.music_play_less,\
personal_assistant.stroka.hibernate,\
personal_assistant.stroka.power_off,\
personal_assistant.stroka.open_start,\
personal_assistant.scenarios.translate,\
personal_assistant.scenarios.random_num,\
personal_assistant.scenarios.music_play_more,\
personal_assistant.scenarios.player_like,\
personal_assistant.scenarios.music_ambient_sound,\
personal_assistant.scenarios.quasar.go_to_the_beginning,\
personal_assistant.stroka.open_history,\
personal_assistant.scenarios.quasar.go_home,\
personal_assistant.scenarios.convert,\
personal_assistant.scenarios.music_what_is_playing,\
personal_assistant.scenarios.sound_louder,\
personal_assistant.scenarios.alarm_set,\
personal_assistant.scenarios.sound_other,\
personal_assistant.handcrafted.future_skill_send_message,\
personal_assistant.scenarios.timer_cancel,\
personal_assistant.stroka.open_ya_browser,\
personal_assistant.stroka.search_local,\
personal_assistant.handcrafted.tell_me_a_joke,\
personal_assistant.handcrafted.recite_a_poem,\
personal_assistant.scenarios.player_replay,\
personal_assistant.scenarios.quasar.go_to_the_end,\
personal_assistant.scenarios.tv_stream,\
personal_assistant.stroka.close_browser,\
personal_assistant.scenarios.quasar.go_forward,\
personal_assistant.scenarios.avia,\
personal_assistant.scenarios.remember_named_location,\
personal_assistant.scenarios.show_route,\
personal_assistant.scenarios.player_shuffle,\
personal_assistant.scenarios.player_repeat,\
personal_assistant.scenarios.tv_broadcast,\
personal_assistant.stroka.open_folder,\
personal_assistant.stroka.open_bookmarks_manager,\
personal_assistant.scenarios.create_reminder,\
personal_assistant.stroka.open_default_browser,\
personal_assistant.stroka.restart_pc,\
personal_assistant.scenarios.get_time,\
personal_assistant.scenarios.sound_unmute,\
personal_assistant.scenarios.find_poi,\
personal_assistant.scenarios.sound_mute,\
personal_assistant.scenarios.get_weather,\
personal_assistant.scenarios.alarm_show,\
personal_assistant.stroka.open_disk \
--forced-intents personal_assistant.scenarios.music_play,\
personal_assistant.scenarios.video_play,\
personal_assistant.scenarios.other \
--dump-dataset-dir $5"

build_reranker_dataset_cmd="echo 'Building tsv dataset' && \
python tools/train/train_reranker.py dump --data-path $2 --is-train $build_reranker_dataset_params && \
python tools/train/train_reranker.py dump --data-path $4 $build_reranker_dataset_params"

share_cmd="cd $5 && \
echo 'Result datasets:' && \
echo 'train.tsv:' && sky share train.tsv && \
echo 'train.cd:' && sky share train.cd && \
echo 'val.tsv:' && sky share val.tsv"

eval "$collect_train_dataset_cmd && $collect_test_dataset_cmd &&
$add_ner_factor_cmd && $add_wizard_factor_cmd && $add_entity_factor_cmd && \
$add_scenarios_lstm_factor_cmd && $add_toloka_lstm_factor_cmd
$build_reranker_dataset_cmd && $share_cmd"
