# Overview
Ниже описывается процесс тренировки [LSTM-классификатора](https://arcanum.yandex-team.ru/arc_vcs/alice/nlu/tools/lstm_classifier_trainer). 
На данный момент поддерживается три режима работы модели: **binary**, **multiclass** и **multilabel**.
Граф тренировки тренирует модель, подбирает пороги на валидационном датасете (для **binary** и **multilabel**), подсчитывает метрики на тестовом датасете и записывает их в Pulsar, находит ошибки классификации, загружает готовую модель в Sandbox. 
Граф подбора гиперпараметров запускает YOPT-эксперимент, подбирающий параметры модели для увеличения качества классификации.

# Workflow

1. Разбиваем датасет на `Train`/`Val`/`Test`.
	Это можно сделать [вот этим графом](https://nirvana.yandex-team.ru/flow/7f5fc824-b41d-476a-b43a-c6cbf6cf1517), заполнив поля `Dataset` сведениями о своём датасете.
	* `Train` будет использован для тренировки LSTM.
	* На `Val` будут подобраны пороги (кроме **multiclass**).
	* На `Test` будут считаться метрики.
1. Запускаем граф тренировки (проверяем валидность датасета/параметров).
1. Запускаем граф подбора гиперпараметров, указав id-шники графа тренировки.


# Граф тренировки

[Граф](https://nirvana.yandex-team.ru/flow/77a9365f-ed1f-4e4b-9318-d412fe3c601d)

Параметры делятся на 4 группы:
* **Токены** – заполняем своими токенами.

* **Dataset** – заполняем данными о датасете.
	- названия колонок с запросом и таргетом укажите в соответствующих параметрах.
	- выбрать режим работы модели можно в опции `model-mode`.
    - параметр `max-token-count` отвечает за фильтрацию слишком длинных строк.

* **Yopt** – используются в графе подбора гиперпараметров, дефолтные параметры подходят.
	- при использовании весов следует отметить опцию `yopt-use-weights`.

* **Util** – служебные поля.
    - `util-embeddings-resource-id` – id ресурса с архивом эмбеддингов (`embeddings.dict`, `embeddings.npy`). Дефолтный ресурс содержит Word2Vec эмбеддинги, при желании можно использовать свои.
    - `util-embeddings-dim` – размерность эмбеддингов из ресурса.
	- `util-upload-to-sandbox` – здесь можно выбрать, хотим ли мы загрузить натренированную модель в Sandbox.
	- `util-sandbox-resource-ttl` – TTL ресурса с моделью.
	- `util-log-every` – отвечает за степень подробности логов кубика Train. Например, при значении `100` будет логироваться каждый 100-й батч.

Из интересного, что можно увидеть в графе:
* Логи кубика `Train` с промежуточными значениями метрик.
* Выход _plots_ кубика `Find Thresholds` с графиками подбора порогов (кроме мультикласса).
* Выход кубиков `Mispreds` с примерами ошибок классификации.
* Выход кубика `Calculate Metrics` с метриками, посчитанными на тестовом датасете.

# Граф подбора гиперпараметров

[Граф](https://nirvana.yandex-team.ru/flow/317e9fab-b036-4fdf-8bfa-a8c86e50314d)

Кубик **Default config** содержит дефолтные параметры, которые подбирать не будем.

Кубик **Space search config** содержит описание перебираемых параметров (дефолт вполне гуд).

В глобальных опциях настраиваем следующие параметры:

* `Nirvana workflow ID` и `Workflow instance ID` – указываем id графа тренировки, их можно получить как из URL, так и из вкладок _Workflow details_ и _Instance details_.
* `model_name` – указываем название модели (`LSTM`).
* `dataset_name` – указываем название датасета.
* `experiment_description` – описание эксперимента.
* `tags` – любые теги, по которым можно будет найти YOPT-эксперимент в Pulsar (например, можно указать свой логин)
* `metric` – метрика по который будут тюниться параметры (пример: `Total.F1`). Все доступные метрики можно посмотреть на выходе кубика `Calculate Metrics` в графе тренировки.
* `tuning_goal` – выбираем вид метрик (больше лучше или меньше лучше).

# Продакшн
В зависимости от вида классификации, в Begemot есть два правила: **AliceBinaryIntentClassifier** и **AliceMultiIntentClassifier**.
Натренированная модель пакуется в кубике `Pack to Sandbox Resource`. 
Для применения классификатора в Begemot следует добавить ресурс для [AliceBinaryIntentClassifier](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc?rev=r9152464#L1) или [AliceMultiIntentClassifier](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/models/alice_multi_intent_classifier/data.inc?rev=r9152464#L1).
* Модель **AliceBinaryIntentClassifier** будет применяться с флагом `bg_lstm_classified_intent=INTENT`, модель **AliceMultiIntentClassifier** будет применяться с флагом `bg_enable_multi_intent_classifier=MODEL`
* Классификаторы прорастают с фрешом (раз в 3 часа). Для использования моделек из фреша нужно добавить флаг `bg_fresh_alice_prefix=INTENT`. UPD: фреш пока что отключен.
* Для того, чтобы использовать новый **AliceBinaryIntentClassifier** или **AliceMultiIntentClassifier** без флагов, нужно добавить интенты в коде [тут](https://a.yandex-team.ru/arc/trunk/arcadia/search/begemot/rules/alice_binary_intent_classifier/alice_binary_intent_classifier.cpp?rev=r9116455#L16) или [тут](https://a.yandex-team.ru/arc/trunk/arcadia/search/begemot/rules/alice_multi_intent_classifier/alice_multi_intent_classifier.cpp?rev=r9148373#L42) соответственно.

# Обратная связь
По всем вопросам можно обращаться в [чат поддержки NLU](https://t.me/+DJryz0GP71djN2Ri).
