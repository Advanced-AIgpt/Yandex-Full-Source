import logging
import os
from typing import List

from tensorflow.keras import Model

from beggins.lib import keras_tools, container
from beggins.lib.config import LearningStage
from beggins.lib.dataset import DatasetRegistry

logger = logging.getLogger(__name__)


def train(stage_configs: List[LearningStage], model: Model, datasets: DatasetRegistry):
    stages = container.OUTPUT.data_file('stages')
    os.makedirs(stages, exist_ok=True)
    for stage_num, learning_stage in enumerate(stage_configs):
        logger.info(f'Starting learning stage {stage_num + 1}...')
        params = {
            'x': datasets[learning_stage.train_dataset].features,
            'y': datasets[learning_stage.train_dataset].target,
            'epochs': learning_stage.epochs,
            'verbose': learning_stage.verbose,
            'batch_size': learning_stage.batch_size,
            'callbacks': [keras_tools.make_callback(callback_config) for callback_config in learning_stage.callbacks],
        }
        if learning_stage.validation_dataset is not None:
            params['validation_data'] = (
                datasets[learning_stage.validation_dataset].features,
                datasets[learning_stage.validation_dataset].target,
            )
            if learning_stage.validation_freq is not None:
                params['validation_freq'] = learning_stage.validation_freq
        logger.info(f'Stage params: {params}')
        model.fit(**params)
        model.save(os.path.join(stages, f'model.stage_{stage_num}.h5'), save_format='h5')
    model.save(container.OUTPUT.state_file('model.h5'))
    return model
