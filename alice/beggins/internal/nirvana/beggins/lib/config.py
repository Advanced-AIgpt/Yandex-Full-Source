import copy
import logging
import os
from typing import List, Optional, Union

import attr
import yaml

logger = logging.getLogger(__name__)


@attr.s()
class BaseComponent:
    name: str = attr.ib()
    params = attr.ib(factory=dict)

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(**dict_)


class Layer(BaseComponent):
    pass


class Optimizer(BaseComponent):
    pass


class Loss(BaseComponent):
    pass


class Metric(BaseComponent):
    pass


@attr.s()
class Model:
    layers: List[Layer] = attr.ib()
    loss: Loss = attr.ib()
    optimizer: Optimizer = attr.ib()
    metrics: List[Metric] = attr.ib()

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(
            layers=[Layer.from_dict(layer) for layer in dict_.get('layers', [])],
            loss=Loss.from_dict(dict_.get('loss', {})),
            optimizer=Optimizer.from_dict(dict_.get('optimizer', {})),
            metrics=[Metric.from_dict(metric) for metric in dict_.get('metrics', [])],
        )


class Callback(BaseComponent):
    pass


@attr.s()
class LearningStage:
    train_dataset: str = attr.ib()
    batch_size: int = attr.ib()
    epochs: int = attr.ib()
    verbose: int = attr.ib(default=2)
    callbacks: List[Callback] = attr.ib(factory=list)
    validation_dataset: Optional[str] = attr.ib(default=None)
    validation_freq: Optional[int] = attr.ib(default=None)

    @classmethod
    def from_dict(cls, dict_: dict):
        dict_ = copy.deepcopy(dict_)
        dict_['callbacks'] = [Callback.from_dict(cb) for cb in dict_.get('callbacks', [])]
        return cls(**dict_)


@attr.s()
class DatasetSource:
    type: str = attr.ib()
    filename: Optional[str] = attr.ib(default=None)

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(**dict_)


@attr.s()
class Dataset:
    name: str = attr.ib()
    source: DatasetSource = attr.ib()

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(
            name=dict_.get('name'),
            source=DatasetSource.from_dict(dict_.get('source', {})),
        )


@attr.s()
class ExportModel:
    name: str = attr.ib()
    filename: str = attr.ib()

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(**dict_)


@attr.s()
class Export:
    models: List[ExportModel] = attr.ib(factory=list)

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(models=[ExportModel.from_dict(model) for model in dict_.get('models', [])])


@attr.s()
class EvalModel:
    name: str = attr.ib()
    filename: str = attr.ib()
    dataset: str = attr.ib()
    batch_size: int = attr.ib(default=1024)

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(**dict_)


@attr.s()
class Eval:
    models: List[EvalModel] = attr.ib(factory=list)

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(models=[EvalModel.from_dict(model) for model in dict_.get('models', [])])


@attr.s()
class Config:
    model: Model = attr.ib()
    learning_stages: List[LearningStage] = attr.ib()
    datasets: List[Dataset] = attr.ib()
    export: Export = attr.ib(factory=Export)
    eval: Eval = attr.ib(factory=Eval)

    @classmethod
    def from_dict(cls, dict_: dict):
        return cls(
            model=Model.from_dict(dict_.get('model', {})),
            learning_stages=[LearningStage.from_dict(ls) for ls in dict_.get('learning_stages', {})],
            datasets=[Dataset.from_dict(ds) for ds in dict_.get('datasets', {})],
            export=Export.from_dict(dict_.get('export', {})),
            eval=Eval.from_dict(dict_.get('eval', {})),
        )


def _try_make_dirs(potential_path):
    prefix = potential_path
    while True:
        prefix, folder = os.path.split(prefix)
        if folder == '':
            # split('/') -> ('/', ''), but at least one folder must be existent
            return
        if os.path.exists(prefix):
            break
    if not os.path.exists(potential_path):
        logger.debug(f'makedirs: {potential_path}')
        os.makedirs(potential_path)


def _update_config_value(value: str, format_map: dict, make_dirs: bool) -> str:
    updated = value.format_map(format_map)
    if make_dirs and updated != value:
        _try_make_dirs(os.path.dirname(updated))
    return updated


def _traverse_config(cfg: Union[list, dict], format_map: dict, make_dirs: bool):
    collection = cfg.items() if isinstance(cfg, dict) else enumerate(cfg)
    for key, value in collection:
        if isinstance(value, str):
            cfg[key] = _update_config_value(value, format_map, make_dirs)
        elif isinstance(value, (dict, list)):
            cfg[key] = _traverse_config(value, format_map, make_dirs)
    return cfg


def read_config(filename: str, format_map: dict = None, make_dirs=True) -> Config:
    with open(filename, 'r') as f:
        cfg = yaml.load(f, yaml.Loader)
    _traverse_config(cfg, format_map or {}, make_dirs)
    return Config.from_dict(cfg)
