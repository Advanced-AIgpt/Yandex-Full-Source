# vins-dm

## Development

### Local enviroment setup

#### Checking out sources

1. Ensure you have an ssh key and it is added to your github account (https://help.github.com/enterprise/2.11/user/articles/checking-for-existing-ssh-keys/)
1. Clone this repo `git clone git@github.yandex-team.ru:vins/vins-dm.git && cd vins-dm && git submodule init && git submodule update` (if you are outside vins team then you should first create fork of vins-dm repository and work with it)
1. Comment out syslog_short logger in api/vins_api/common/settings.py config (it causes troubles in development, but is not needed for it)

#### Creating virtual environment

##### Using Virtualbox + Vagrant

1. Install [Vagrant](https://www.vagrantup.com/)
   and [Virtualbox](https://www.virtualbox.org/)
1. `cd vins-dm`
1. `vagrant up`
1. `vagrant ssh`
1. `sudo apt-get install language-pack-ru`
1. Inside virtualbox setup your comfortable enviroment (emacs, zsh, etc.) if needed.
1. Create virtualenv `virtualenv ~/vins-venv`

##### Using Docker

Warning: docker has problems on Mac OS with ipv6 in containers - you won't be able to rebuild model locally.

1. Install [Docker](https://store.docker.com/search?offering=community&type=edition)
1. Increase memory available to docker to 5GB. Allowing docker to use all CPUs is also a good idea.
1. Go to directory with your local vins-dm repo and build docker image `docker build -f dockerfiles/Dockerfile.dev -t ubuntu_vins_dev .`
1. Run container for the first time with `docker run -it --name=ubuntu_vins -p 8000:8000 -v ~:/host_home ubuntu_vins_dev bash -i`
for further runs create script ubuntu_vins.sh
```
#!/bin/sh
docker restart ubuntu_vins && docker attach ubuntu_vins
```
5. Link your repo to root's home directory `ln -s /host_home/git/vins-dm /root/vins-dm`

#### Final confguration and check of virtual environment

1. Activate virtualenv `. ~/vins-venv/bin/activate`
1. `cd vins-dm`
1. Update pip `pip install -U pip`
1. Setup python requirements `bash env_scripts/setup-venv.sh`
1. Run tests `py.test`
1. Run syntax checker `flake8`

If you add new binary dependencies please don't forget to add them to
`env_scripts/setup-env.sh` script.

#### Troubleshooting

*- Git reports error `Pointer file error: Unable to parse pointer at: <path>` on `git checkout` or `git pull` and the error doesn't go.*
* Open the &lt;git-root&gt;/.gitattributes file in an enditor
* Remove lines matching &lt;path&gt; from the error message
* Close editor
* `git checkout HEAD -- <path>`
* `git checkout HEAD -- .gitattributes`

### CI
If you add new binary dependencies to project you probably want to add
them to the TeamCity-agent machine that executes tests.

1. Go to the https://cloud.yandex-team.ru/
2. Select `VoiceInterfaces` project
3. Go to `Images`
4. Run new instance from image `vins-test-deps` with some `<name>` and
   `SHARESANDBOX` network.
5. `ssh -A <name>.haze.yandex.net` after instance is up
6. install new dependencies
7. Go to the https://cloud.yandex-team.ru/
8. Make instance snapshot and save it as `vins-test-deps`
9. After snpshot is ready destroy your instance


### Rebuild model

#### Locally

To rebuild NLU model use `tools/nlu/compile_app_model.py`:
```
usage: compile_app_model.py [-h] [--app APP] [--all]

optional arguments:
  -h, --help  show this help message and exit
  --app APP   The name of the app to compile models for, multiple names can be
              specified.
  --all       Compile all apps. (personal_assistant, navi_app)
  --update-config   Rewrite model hash sum in config file
  --classifiers clf_1[ clf_2 clf_3 ...] Specify specific classifiers to train, or omit this option to train all classifiers
  --taggers     Use this flag to train taggers
  --skip-train  Skip all training steps. This could be useful when compiling classifiers / taggers with modified attributes, that doesn't affect model's training stage
  --feature-cache PATH  Create feature cache file. Next rerun will use this cache and skip related preprocessing steps.
```

Do not forget to set environment variables like follows:
* `VINS_NUM_PROCS=32`
* `OMP_NUM_THREADS=1`

Example:
```
python tools/nlu/compile_app_model.py --app personal_assistant --update-config
```

After successful compilation update appropriate `Vinsfile.json`
`nlu.compiled_model` config section with new sha256 checksum.

#### Rebuilding scenarios for personal_assistant app
When you make changes at **scenarios** level in personal_assistant app, it is useful to retrain metric model to enhance classification performance
Metric learning is running by
```angular2html
./tools/pa/metric_learning.sh --feature-cache <path-to-feature-cache>
```
After metric learning finished, launch model building at scenarios level:
```angular2html
python tools/nlu/compile_app_model.py --app personal_assistant --update-config --classifiers scenarios --feature-cache <path-to-feature-cache>
```

#### FAQ

*- Running compile_app_model script takes too long time. Is there any way to speed up the process?*
* Ensure to use as much cores as possible, e.g. by using `VINS_NUM_PROCS=32` variable
* The most costly part of model compilation is preprocessing  (feature extraction). It is possible to cache precomputed features in order to use them on the next run by running script with the flag `--feature-cache path/to/cache_file.pkl`.
* If your changes affect only one level of intent classifier's cascade (e.g., applying changes only to one intent is usually the case), there is no need to retrain other levels. Therefore specify target level by using `--classifiers <cascade level name>`. (Cascade level name could be found in intent configuration file under `"trainable_classifiers"` field)

*- Taggers are not compiled properly*

Do not forget to use `--taggers` option when recompiling taggers.

*- I have successfully recompiled my model. How to test my changes?*

Either use `process_nlu_on_dataset.py` script (see bellow) to check whether your changes affect performance metric, or chat with telegram bot. Personal assistant Telegram bot istance is deployed locally by running
```
python apps/personal_assistant/personal_assistant/pa_bot.py --telegram_token <your-telegram-token>
```
Telegram token could be requested from [BotFather](https://core.telegram.org/bots#6-botfather)

*- I have added new phrases to my-new-intent.nlu, but I can't hit them exactly when chatting with telegram bot or any other deployed VINS instance.*
* Check if the same phrases are not included in any other intents
* Since model features are based on external resources, it sometimes happens that they become mismatched after compiling due to external changes. If you are using `--feature-cache` flag, try to remove feature cache file and rebuild it again.

*- Why intent classification performance for my scenario in personal_assistant app is so poor?*

* Try to [rebuild metric model](https://github.yandex-team.ru/vins/vins-dm#rebuilding-scenarios-for-personal_assistant-app)
* Try to add new custom entities
* Try to [find false neighbors in your *.nlu files](https://github.yandex-team.ru/vins/vins-dm#find-false-neighbors-inside-knn-model)

### Validate model performance

#### Validate intent classification

To check model performance on predefined validation dataset `pa_scenarios.tsv`, use `tools/nlu/process_nlu_on_dataset.py`. Example:
```
VINS_NUM_PROCS=32 CUDA_VISIBLE_DEVICES=-1 python tools/nlu/process_nlu_on_dataset.py personal_assistant classify apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intents_unite_with_music.tsv -o nlu.output.pkl --prev-intent-col prev_intent --app-info='{"app_id": "winsearchbar", "platform": "windows"}' --experiments="video_play,how_much"
```
This command creates file `nlu.output.pkl`, that is basically pickled semantic frames & ground truth data per utterance. Next step is to use it to compute performance metrics:
```
python tools/nlu/process_nlu_on_dataset.py personal_assistant report nlu.output.pkl --rename=apps/personal_assistant/personal_assistant/tests/validation_sets/toloka_intent_renames.json --errors --other=.*other
```
Successful run outputs something similar to:
```
Report:
                                                                 precision    recall  f1-score   support

       personal_assistant.handcrafted.future_skill_send_message       1.00      0.08      0.15        12
                   personal_assistant.handcrafted.recite_a_poem       1.00      0.42      0.59        50
                            personal_assistant.handcrafted.rude       0.53      0.73      0.61       408
                                                ......
                           personal_assistant.stroka.restart_pc       1.00      0.89      0.94        18
                          personal_assistant.stroka.restore_tab       0.00      0.00      0.00         3
                         personal_assistant.stroka.search_local       0.00      0.00      0.00         2

                                                    avg / total       0.95      0.94      0.94     60977

Accuracy: 0.944290
Macro F-score: 0.607912
```
#### Validate slot tagging

To evaluate quality of slot tagging, the input file `tools/nlu/process_nlu_on_dataset.py` must be run on a tagged "ground truth" `.nlu` file. Typically, such files contain a single intent. The classifier can be forced to predict this intent with the `--force-intent` option for the `classify` command. For example, to evaluate tagging of video, you can first run
```
python tools/nlu/process_nlu_on_dataset.py personal_assistant classify apps/personal_assistant/personal_assistant/tests/validation_sets/video/video_tagged.nlu --format nlu -o tagger.output.pkl --force-intent personal_assistant.scenarios.video_play
```
Then you can produce the resulting markup file and report of tagging quality by applying options `--nlu-markup` and `--recall-by-slots` to the `report` command, like in
```
python tools/nlu/process_nlu_on_dataset.py personal_assistant report tagger.output.pkl --nlu-markup --recall-by-slots
```
Successful run outputs something similar to:
```
Slot recalls (accuracy of the best attempt):
recall@1: 0.7157
recall@2: 0.7919
recall@10: 0.8731
recall@5: 0.8731
Slot "personal_assistant.scenarios.video_play:episode" recalls:
recall@1: 0.8235
recall@2: 0.8235
recall@10: 0.8824
recall@5: 0.8824
Slot "personal_assistant.scenarios.video_play:search_text" recalls:
......
Slot "personal_assistant.scenarios.video_play:new" precision@1: 0.8000
Mean slot recalls:
mean slot recall@1: 0.8271
mean slot recall@2: 0.8786
mean slot recall@10: 0.9228
mean slot recall@5: 0.9223
Mean slot precision@1:0.8681
```

### Word embeddings pool

| sandbox_id  | size  | window  | mode  | data description |
|:---:|:---:|:---:|:---:|:---:|
| 233337600  |  100 | 2  | cbow  |  voice jan-feb |
| 233338991  | 100  | 2  | skipgram  | voice jan-feb  |
| 233339497  | 300  | 1 | cbow  | voice jan-feb  |
| 233340523 | 300 | 2 | cbow | voice jan-feb |
| 233341128 | 300 | 3 | cbow | voice jan-feb |
| 233342178 | 300 | 1 | skipgram | voice jan-feb |
| 233342907| 300 | 2 | skipgram | voice jan-feb |
| 233343757 | 300 | 3 | skipgram | voice jan-feb |
| 233344986 | 300 | 2 | skipgram | voice fall+jan-feb |


### Anaphora resolution

There are two possible mechanism to resolve anaphora in `vins_core`:

1. Resolving anaphora by matching pronouns with entities and changing the user's utterance accordingly. To use this mechanism, perform two steps:
    * Add to `"nlu"` section of your `Vinsfile.json` the allowed intents for using anaphora resolved utterance:
        ```
        "nlu": {
            "anaphora_resolution": {
              "intents": [
                "personal_assistant.scenarios.search",
                "personal_assistant.scenarios.get_weather",
                "personal_assistant.scenarios.convert"
              ]
            },
            "feature_extractors": [
              {
                "id": "word",
             ...
        ```
    * Add `"anaphora_resolver"` sample processor to your `"samples_extractor"` config. Note that anaphora resolver uses `"wizard"` sample processor, so you must include it as well:
        ```
        "samples_extractor": {
            "pipeline": [
                ...
                {
                    "name": "wizard"
                },
                ...
                {
                    "name": "anaphora_resolver",
                    "max_utterances": 2
                }
            ]
         }

        ```
2. Using entity search to find objects in context and then import them into slots of your form. To use this mechanism, you need:
    * Add `"entitysearch"` sample processor to your `"samples_extractor"` config. Note that it uses `"wizard"` sample processor, so you must include it as well:
        ```
        "samples_extractor": {
            "pipeline": [
                ...
                {
                    "name": "wizard"
                },
                ...
                {
                    "name": "entitysearch"
                }
            ]
         }
        ```
    * Specify `"import_tags"` and/or `"import_types"` for slots you want import entities to. Also, you **must** specify `"import_entity_pronouns"` for slot. Read more in [DIALOG-1266](https://st.yandex-team.ru/DIALOG-1266#1518177206000), look for examples in personal_assistant scenarios or ask `@persiyanov`.

#### TeamCity

1. Push your changes to git branch
1. Run [TeamCity task](https://teamcity.yandex-team.ru/viewType.html?buildTypeId=VoiceServer_VinsDm_CompileNluModel) on this branch


### Stubs download

For new integrational tests you need to download and commit stubs (saved bass respose for tests to use it in mocks).

Download all:
```
python tools/pa/download_bass_stubs.py --dump-dir apps/personal_assistant/personal_assistant/tests/integration_data/stubs/
```

Download test by name
```
python tools/pa/download_bass_stubs.py --dump-dir apps/personal_assistant/personal_assistant/tests/integration_data/stubs/ --test-case test_currency<::test_one>
```

```
usage: download_bass_stubs.py [-h] [--test-case SPEC] [--bass-url URL]
                              [--dump-dir DIR]

optional arguments:
  -h, --help        show this help message and exit
  --test-case SPEC  Test case spec. Format: "test_file::test_name" or
                    "test_file". If omited stubs will be created for all
                    integration tests
  --bass-url URL    BASS URL, optional
  --dump-dir DIR    Directory for BASS responses dump
```

### Analytic tools
#### Visualizing embeddings inside KNN model
In order to use tensorboard *Embeddings* visualizer, run
```angular2html
python tools/nlu/app_analyzer.py knn_embeddings personal_assistant scenarios --logdir=<tensorboard-logdir> --points-per-intent=500
```
then run [tensorboard](https://www.tensorflow.org/programmers_guide/summaries_and_tensorboard)

#### Find false neighbors inside KNN model
One way to enhance intent classification performance is to check whether the train set contains erroneous examples that lead to false neighbors at inference.
Try to run
```angular2html
OMP_NUM_THREADS=32 python tools/nlu/app_analyzer.py false_knn_embeddings personal_assistant scenarios --intents="<regular expression to match specific intents>" --output-dir=<output-dir>
```
In directory &lt;output-dir&gt;, the following info could be found:
* &lt;output-dir&gt;/stats.json with total error statistics
* multiple &lt;output-dir&gt;/counts/&lt;intent-name&gt;/&lt;false-neighbor-intent-name&gt;.tsv that store utterances of intent &lt;false-neighbor-intent-name&gt; that spoil classification of utterances from &lt;intent-name&gt; and their error rates
* multiple &lt;output-dir&gt;/scores/&lt;intent-name&gt;.tsv that store misclassified utterances of &lt;intent-name&gt; and mean cosine similarity to their false neighbors
There is where_is_this_phrase-tool to calculate sources of false neighbors (their original text and nlu path). It requires either feature_cache (runs slower) or *.text_to_source file (runs faster). *.text_to_source file is calculated on every model train and stored at %%feature_cache_path%%.text_to_source
