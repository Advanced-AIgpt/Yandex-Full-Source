This folder contains validation sets for tagging several intents. Files should be named `intent_name.nlu`.

They are processed by TC task 'Measure PA NLU quality (vins)' with a script like this:

```
#!/bin/bash

EXPERIMENTS=video_play,how_much

if [[ -d "apps/personal_assistant/personal_assistant/tests/validation_sets/tagger_validation" ]]; then
  FILES=apps/personal_assistant/personal_assistant/tests/validation_sets/tagger_validation/*.nlu
  for f in $FILES
  do
    echo "Processing $(basename $f .txt) file..."
    VINS_SKILLS_ENV_TYPE=beta python tools/nlu/process_nlu_on_dataset.py personal_assistant classify -o activations.output.pkl $f --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments=$EXPERIMENTS --format nlu -o tagger.$(basename $f .nlu).pkl --force-intent $(basename $f .nlu)
  done
fi


if [[ -d "apps/personal_assistant/personal_assistant/tests/validation_sets/tagger_validation" ]]; then
  FILES=apps/personal_assistant/personal_assistant/tests/validation_sets/tagger_validation/*.nlu
  for f in $FILES
  do
    echo "Reporting $(basename $f .txt) file..."
    python tools/nlu/process_nlu_on_dataset.py personal_assistant report tagger.$(basename $f .nlu).pkl --nlu-markup --recall-by-slots --recalls-at 1 --common-tagger-errors --num-most-common-errors 30
  done
fi
```
