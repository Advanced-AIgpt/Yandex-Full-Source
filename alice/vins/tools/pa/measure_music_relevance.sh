#!/usr/bin/env bash

# search_relevance_basket.tsv

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/search_relevance_basket.tsv \
       --output-file search_relevance_PA_mode.tsv \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/search_relevance_basket.tsv \
       --output-file search_relevance_PA_mode_websearch.tsv \
       --format tsv \
       --experiments music_use_websearch \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/search_relevance_basket.tsv \
       --output-file search_relevance_quasar_mode.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_show_first_track \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/search_relevance_basket.tsv \
       --output-file search_relevance_quasar_mode_websearch.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_use_websearch music_show_first_track \
       --format tsv \
       --slots answer

# reported_bugs_with_switch_on_prefix.tsv

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_with_switch_on_prefix.tsv \
       --output-file reported_bugs_with_switch_on_prefix_PA_mode.tsv \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_with_switch_on_prefix.tsv \
       --output-file reported_bugs_with_switch_on_prefix_PA_mode_websearch.tsv \
       --experiments music_use_websearch \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_with_switch_on_prefix.tsv \
       --output-file reported_bugs_with_switch_on_prefix_quasar_mode.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_show_first_track \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_with_switch_on_prefix.tsv \
       --output-file reported_bugs_with_switch_on_prefix_quasar_mode_websearch.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_use_websearch music_show_first_track \
       --format tsv \
       --slots answer

# reported_bugs_without_switch_on_prefix.tsv

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_without_switch_on_prefix.tsv \
       --output-file reported_bugs_without_switch_on_prefix_PA_mode.tsv \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_without_switch_on_prefix.tsv \
       --output-file reported_bugs_without_switch_on_prefix_PA_mode_websearch.tsv \
       --experiments music_use_websearch \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_without_switch_on_prefix.tsv \
       --output-file reported_bugs_without_switch_on_prefix_quasar_mode.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_show_first_track \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_without_switch_on_prefix.tsv \
       --output-file reported_bugs_without_switch_on_prefix_quasar_mode_websearch.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_use_websearch music_show_first_track \
       --format tsv \
       --slots answer

# music_top_cleaned.tsv

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_top_cleaned.tsv \
       --output-file music_top_cleaned_PA_mode.tsv \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_top_cleaned.tsv \
       --output-file music_top_cleaned_PA_mode_websearch.tsv \
       --experiments music_use_websearch \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_top_cleaned.tsv \
       --output-file music_top_cleaned_quasar_mode.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_show_first_track \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_top_cleaned.tsv \
       --output-file music_top_cleaned_quasar_mode_websearch.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_use_websearch music_show_first_track \
       --format tsv \
       --slots answer

# music_unique_cleaned.tsv

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_unique_cleaned.tsv \
       --output-file music_unique_cleaned_PA_mode.tsv \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_unique_cleaned.tsv \
       --output-file music_unique_cleaned_PA_mode_websearch.tsv \
       --experiments music_use_websearch \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_unique_cleaned.tsv \
       --output-file music_unique_cleaned_quasar_mode.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_show_first_track \
       --format tsv \
       --slots answer

python tools/process_app_on_dataset.py --app personal_assistant \
       --input-files apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_unique_cleaned.tsv \
       --output-file music_unique_cleaned_quasar_mode_websearch.tsv \
       --test-user-tags oauth \
       --app-info '{"app_id": "ru.yandex.quasar.vins_test", "app_version": "1.0", "os_version": "6.0.1", "platform": "android"}' \
       --experiments music_no_enqueue music_use_websearch music_show_first_track \
       --format tsv \
       --slots answer
