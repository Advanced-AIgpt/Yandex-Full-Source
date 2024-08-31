import json

import tensorflow as tf

from beggins.lib import config, container
from beggins.lib.dataset import DatasetRegistry


def evaluate(cfg: config.Eval, datasets: DatasetRegistry):
    evaluation_result = {}
    for model_config in cfg.models:
        tf.keras.backend.clear_session()
        model: tf.keras.Model = tf.keras.models.load_model(model_config.filename)
        x = datasets[model_config.dataset].features
        metrics = model.evaluate(
            x=x,
            y=datasets[model_config.dataset].target,
            batch_size=min(len(x), model_config.batch_size),
        )
        metrics = list(map(float, metrics))
        evaluation_result[model_config.name] = dict(zip(model.metrics_names, metrics))
    with open(container.OUTPUT.json_output, 'w') as f:
        json.dump(evaluation_result, f, ensure_ascii=False, indent=4)
