from typing import List

from tensorflow import keras

from beggins.lib import config


def _make_base_component(module, component_config: config.BaseComponent):
    cls = getattr(module, component_config.name)
    return cls(**component_config.params)


def _make_base_components(module, component_configs: List[config.BaseComponent]):
    components = []
    for component_config in component_configs:
        components.append(_make_base_component(module, component_config))
    return components


def _make_layers(layer_configs: List[config.Layer]):
    return _make_base_components(keras.layers, layer_configs)


def _make_loss(loss_config: config.Loss):
    return _make_base_component(keras.losses, loss_config)


def _make_optimizer(optimizer_config: config.Optimizer):
    return _make_base_component(keras.optimizers, optimizer_config)


def _make_metrics(metric_configs: List[config.Metric]):
    return _make_base_components(keras.metrics, metric_configs)


def make_model(model_config: config.Model):
    model = keras.Sequential(layers=_make_layers(model_config.layers))
    model.compile(
        optimizer=_make_optimizer(model_config.optimizer),
        loss=_make_loss(model_config.loss),
        metrics=_make_metrics(model_config.metrics),
    )
    return model


def make_callback(callback_config: config.Callback):
    return _make_base_component(keras.callbacks, callback_config)
