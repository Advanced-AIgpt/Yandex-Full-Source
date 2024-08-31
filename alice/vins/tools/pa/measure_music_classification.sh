#!/usr/bin/env bash

python tools/nlu/process_nlu_on_dataset.py personal_assistant classify \
       apps/personal_assistant/personal_assistant/tests/validation_sets/music/search_relevance_basket.tsv \
       -o search_relevance_basket.output.pkl --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=video_play
python tools/nlu/process_nlu_on_dataset.py personal_assistant report search_relevance_basket.output.pkl --errors
python tools/nlu/process_nlu_on_dataset.py personal_assistant classify \
       apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_with_switch_on_prefix.tsv \
       -o reported_bugs_with_switch_on_prefix.output.pkl --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=video_play
python tools/nlu/process_nlu_on_dataset.py personal_assistant report reported_bugs_with_switch_on_prefix.output.pkl --errors
python tools/nlu/process_nlu_on_dataset.py personal_assistant classify \
       apps/personal_assistant/personal_assistant/tests/validation_sets/music/reported_bugs_without_switch_on_prefix.tsv \
       -o reported_bugs_without_switch_on_prefix.output.pkl --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=video_play
python tools/nlu/process_nlu_on_dataset.py personal_assistant classify \
       apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_top_cleaned.tsv \
       -o music_top_cleaned.output.pkl --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=video_play
python tools/nlu/process_nlu_on_dataset.py personal_assistant report music_top_cleaned.output.pkl --errors
python tools/nlu/process_nlu_on_dataset.py personal_assistant classify \
       apps/personal_assistant/personal_assistant/tests/validation_sets/music/music_unique_cleaned.tsv \
       -o music_unique_cleaned.output.pkl --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=video_play
python tools/nlu/process_nlu_on_dataset.py personal_assistant report music_unique_cleaned.output.pkl --errors
python tools/nlu/process_nlu_on_dataset.py personal_assistant classify \
       apps/personal_assistant/personal_assistant/tests/validation_sets/music/kluv.tsv \
       -o kluv.output.pkl --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=video_play
python tools/nlu/process_nlu_on_dataset.py personal_assistant report kluv.output.pkl --errors

python -c "import json; name='search_relevance_basket';print('%s: %s' % (name, json.load(open(name+'.output.intent.metrics.json'))['report']['personal_assistant.scenarios.music_play']['recall']))"
python -c "import json; name='reported_bugs_with_switch_on_prefix';print('%s: %s' % (name, json.load(open(name+'.output.intent.metrics.json'))['report']['personal_assistant.scenarios.music_play']['recall']))"
python -c "import json; name='reported_bugs_without_switch_on_prefix';print('%s: %s' % (name, json.load(open(name+'.output.intent.metrics.json'))['report']['personal_assistant.scenarios.music_play']['recall']))"
python -c "import json; name='music_top_cleaned';print('%s: %s' % (name, json.load(open(name+'.output.intent.metrics.json'))['report']['personal_assistant.scenarios.music_play']['recall']))"
python -c "import json; name='music_unique_cleaned';print('%s: %s' % (name, json.load(open(name+'.output.intent.metrics.json'))['report']['personal_assistant.scenarios.music_play']['recall']))"
python -c "import json; name='kluv';print('%s: %s' % (name, json.load(open(name+'.output.intent.metrics.json'))['report']['personal_assistant.scenarios.music_play']['recall']))"
