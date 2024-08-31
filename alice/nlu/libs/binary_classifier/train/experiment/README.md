# Binary Classifier Experiments

Set of training scripts for experiments on a binary preclassifier.

Ticket: [DIALOG-5811](https://st.yandex-team.ru/DIALOG-5811).

## Data Preparation
(It's up to be refactored into a single vh script)

Run `ya m -r --checkout && ./binary_classifier_trainer --table-path <path to table>`

Then join boltalka embeddings like [there](https://nirvana.yandex-team.ru/flow/5bd1a01e-5f7f-4ee2-aff6-69f00c3f7c14/290f9f52-1d45-4b24-ab8d-da449d996bcd/graph`).

Prepared data can be found [here](https://yt.yandex-team.ru/hahn/navigation?path=//home/voice/dan-anastasev/nlu_data&offsetMode=row). Train/validation split is [here](https://yt.yandex-team.ru/hahn/navigation?path=//home/voice/dan-anastasev/intent_binary_classifier&).
(The split is obtained with following [yql operation](https://yql.yandex-team.ru/Operations/Xd6YEglcTp9WrGneApUT5O8jUsmS6QkW_FOLp-fkYGw=) - 70% unique texts goes to train part, everything else is in validation one).

## Train
### Linear Models
Run: `python linear_model.py --dataset-config-path <>`.

It will train logistic regression models with dssm and tf-idf vectorizers and save them with their metrics in the `models` folder.
