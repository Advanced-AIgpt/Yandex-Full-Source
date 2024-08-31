import json
import os
import random

from functools import partial
from multiprocessing import Array

from alice.uniproxy.library.event_patcher import EventPatcher, MultiPatcher

from alice.uniproxy.library.logging import Logger
import alice.uniproxy.library.settings as settings


class Experiment:
    def __init__(self, cfg, macros=None, share_getter=None):
        if macros is None:
            macros = {}
        self.id = cfg['id']
        self._share_getter = share_getter
        self._apply_share = cfg['share']

        self.event_patcher = EventPatcher(cfg['flags'], macros)

    @property
    def apply_share(self):
        if self._share_getter:
            return self._share_getter()
        else:
            return self._apply_share

    @property
    def cfg_share(self):
        return self._apply_share


def log_experiment(unisystem, experiment):
    try:
        unisystem.log_experiment({
            'id': experiment.id,
            'type': 'local',
            'control': False,
        })
    except ReferenceError:  # ignore weakly unisystem
        pass


def check_if_experiment_usable(experiment, random_seed=None):
    random.seed(random_seed)
    return bool(random.random() < experiment.apply_share)


class Experiments:
    @staticmethod
    def experiments_from_directory(exp_directory):
        experiments = []
        try:
            for fn in os.listdir(exp_directory):
                if fn.endswith(".json"):
                    full_fn = os.path.join(exp_directory, fn)
                    try:
                        with open(full_fn) as f:
                            experiments.append(json.load(f))
                    except Exception as err:
                        Logger.get().error('can not load JSON with experiment from {}: {}'.format(full_fn, repr(err)))
        except OSError as err:
            Logger.get().warning('can not load experiments configuration from {}: {}'.format(exp_directory, err))
        return experiments

    def __init__(self, experiments, macros=None, mutable_shares=False):
        if macros is None:
            macros = {}
        self.qloud_instance = os.getenv('QLOUD_DISCOVERY_INSTANCE')
        self.exps = []
        self._mutable_shares = None
        self._mutable_shares_index = {}  # Experiment ID -> Experiment index

        if not isinstance(experiments, (list, tuple)):
            # parse all files from experiments' directory with name matches *.json template
            experiments = self.experiments_from_directory(experiments)

        exp_idx = 0
        if mutable_shares:
            self._mutable_shares = Array('d', len(experiments))
        for exp in experiments:
            try:
                get_share = None
                if self._mutable_shares:
                    self._mutable_shares[exp_idx] = exp['share']
                    get_share = partial(lambda x: self._mutable_shares[x], exp_idx)
                experiment = Experiment(exp, macros, share_getter=get_share)
                self._mutable_shares_index[experiment.id] = exp_idx
                exp_idx += 1
                self.exps.append(experiment)
            except Exception as err:
                Logger.get().error('can not load experiment from config: {}'.format(repr(err)))

    def set_share(self, exp_id, share):
        if not self._mutable_shares:
            raise Exception('changing experiments share not enabled')

        idx = self._mutable_shares_index.get(exp_id)
        if idx is None:
            raise Exception('not found experiment=<{}>'.format(exp_id))

        self._mutable_shares[idx] = share

    def get_share(self, exp_id):
        if not self._mutable_shares:
            raise Exception('changing experiments share not enabled')

        idx = self._mutable_shares_index.get(exp_id)
        if idx is None:
            raise Exception('not found experiment=<{}>'.format(exp_id))

        return self._mutable_shares[idx]

    def try_use_experiment(self, unisystem):
        try:
            return self.try_use_experiment_(unisystem)
        except ReferenceError:
            return False

    def try_use_experiment_(self, unisystem):
        patcher = MultiPatcher()

        for exp in self.exps:
            if not (unisystem.exps_check or check_if_experiment_usable(exp, unisystem.session_data.get('uuid'))):
                continue
            patcher.add_patcher(exp.event_patcher, exp=exp)
            log_experiment(unisystem, experiment=exp)

        if patcher.empty:
            return False

        unisystem.set_event_patcher(patcher)
        return True


_exp_config_ = settings.config.get("experiments", {})
experiments = Experiments(
    _exp_config_.get("list", _exp_config_.get("cfg_folder")),
    _exp_config_.get("macros", {}),
    mutable_shares=_exp_config_.get('mutable_shares', True),
)
