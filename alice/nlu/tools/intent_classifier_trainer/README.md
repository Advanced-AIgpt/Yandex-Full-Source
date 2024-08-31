# Binary Classifier Trainer

Нужен для обучения и тестирования [бинарного классификатора](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/libs/binary_classifier/ya.make) для распознавания интентов.

## Процесс обучения
Для обучения нужно прописать конфиг и отправить его как параметр в исполняемый файл: `./classifier_granet --config-path <путь до вашего конфига>`. Все пути в конфиге должны быть прописаны относительно конфига. [Пример](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/train/configs/sound_louder/train.json).

1. Для режима обучения в конфиге необходимо указать `"mode": "fit"`.
2. Нужно прописать пути до датасетов с положительными (`positives_path`) и отрицательными (`negatives_path`) сэмплами в tsv формате. Если в них есть колонка `embeddings`, то эмбеддинги будут браться оттуда, иначе они будут искаться в [файле c эмбеддингами](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/tools/intent_classifier_trainer/internal_data/ya.make#L6). Его можно изменить, прописав путь `embeddings_path`.
3. __Обязательно__ прописать параметр `model_path` до директории, где будет сохранена обученная модель. После процесса обучения в этой директории появятся две папки: `checkpoint` и `proto`. Первая нужна для использования этой утилиты для быстрого тестирования полученной модели. Во второй лежит сохраненная модель в формате `protobuf`, именно эту директорию нужно будет передавать [бинарному классификатору в проде](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/libs/binary_classifier/ya.make).
4. Также можно указывать параметры обучения модели: `hidden_layer_sizes` для количества нейронов в слоях, `batch_size` для размера батча и `epoch_count` для задания количества эпох обучения.

## Процесс тестирования
Может быть использован для подсчета стандартных метрик (`presicion`, `recall`, `f1`) и для получения предсказанных вероятностей для тестовых датасетов (смотри пункт 4).
1. Для режима тестирования в конфиге необходимо указать `"mode": "predict"`.
2. Нужно прописать пути до датасетов с положительными (`positives_path`) и отрицательными (`negatives_path`) сэмплами в tsv формате. В отличие от режима обучения, при желании можно указать только один из этих файлов. При этом, для них действует то же правило с эмбеддингами, что было описано в соответствующем пункте режима обучения.
3. __Обязательно__ задать параметр `model_path` до директории с моделью, обученной с помощью обучающего режима.
4. При желании можно задать параметр `result_path`. В таком случае по этому пути будет создан tsv-файл с предсказанными вероятностями (колонка `predicted`) и правильным ответом (колонка `real`) для всех сэмплов из файлов `positives_path` и `negatives_path`. 

## Попробовать обученную модель в бегемоте
1. Обучить модель согласно описанному процессу. 
2. Залить модель по вашему пути на sandbox с помощью команды `ya upload --tar <путь к вашей предобученной модели>`. В случае успеха вы получите ответ: `Download link: https://proxy.sandbox.yandex-team.ru/42`. Нужно запомнить `id` загрузки (в данном случае это `42`).
3. Пройти к файлу `$ARCADIA/search/wizard/data/wizard/AliceBinaryIntentClassifier/ya.make` и дописать конфиг:
```
# измените sound_louder на название своей модели
FROM_SANDBOX(
    42
    RENAME
    sound_louder/proto/model.pb
    sound_louder/proto/model_description.json
    OUT_NOAUTO
    dssm_models/sound_louder/model.pb
    dssm_models/sound_louder/model_description.json
)
```
4. [Скомпилить бегемот.](https://wiki.yandex-team.ru/users/samoylovboris/begemot/)
5. Задавать запросы согласно [сниппетам](https://wiki.yandex-team.ru/users/samoylovboris/begemot/) или [документации](https://wiki.yandex-team.ru/begemot/#poslatzaprosnadevmashinucgi-parametromadresnojjstroki).
