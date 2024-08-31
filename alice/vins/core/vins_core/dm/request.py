# coding: utf-8

from __future__ import unicode_literals

from uuid import uuid4

import attr
import md5

from vins_core.common.utterance import Utterance
from vins_core.utils.datetime import utcnow
from vins_core.utils.config import get_setting
from .request_events import TextInputEvent, ServerActionEvent, RequestEvent


@attr.s(slots=True, frozen=True)
class AppInfo(object):
    """ Class for collect application info from request """
    app_id = attr.ib(default=None)
    app_version = attr.ib(default=None)
    os_version = attr.ib(default=None)
    platform = attr.ib(default=None)
    device_manufacturer = attr.ib(default=None)
    device_model = attr.ib(default=None)

    def to_dict(self):
        return attr.asdict(self)


def _event_validator(inst, attr, value):
    if value is not None and not isinstance(value, RequestEvent):
        raise TypeError("%s must be RequestEvent or None, got %s" % (attr.name, type(value)))


class Experiments(object):
    ENABLED_VALUE = '1'

    def __init__(self, experiments=None):
        if not experiments:
            self._experiments = {}
        elif isinstance(experiments, Experiments):
            self._experiments = experiments._experiments.copy()
        elif isinstance(experiments, dict):
            self._experiments = experiments.copy()
        else:
            self._experiments = {key: Experiments.ENABLED_VALUE for key in experiments}

    def items(self):
        return self._experiments.iteritems()

    def __contains__(self, item):
        raise DeprecationWarning('You should check value explicitly: experiments[\'%s\']' % item)

    def __iter__(self):
        raise DeprecationWarning('You should use experiments.items()')

    def __copy__(self):
        return Experiments(self)

    def __getitem__(self, item):
        return self._experiments.get(item)

    def merge(self, experiments):
        new_experiments = Experiments(self)
        new_experiments._experiments.update(experiments)
        return new_experiments

    def to_dict(self):
        return self._experiments.copy()


def configure_experiment_flags(current_experiments, experiment_modifiers):
    if isinstance(experiment_modifiers, dict):
        return current_experiments.merge(experiment_modifiers)

    return current_experiments.merge({key: Experiments.ENABLED_VALUE for key in experiment_modifiers})


def get_experiments():
    result = {}
    for experiment in get_setting('EXPERIMENTS', '').split(','):
        experiment = experiment.split('=', 1)
        experiment_name = experiment[0].strip()
        if experiment_name:
            if len(experiment) > 1:
                result[experiment_name] = experiment[1].strip()
            else:
                result[experiment_name] = Experiments.ENABLED_VALUE
    return Experiments(result)


@attr.s(slots=True, frozen=True)
class ReqInfo(object):
    """ Class for collect parameters from request """
    uuid = attr.ib()
    client_time = attr.ib()
    app_info = attr.ib()
    utterance = attr.ib(default=None)
    lang = attr.ib(default='ru-RU')
    user_lang = attr.ib(default='ru')
    location = attr.ib(default=None)
    reset_session = attr.ib(default=False)
    experiments = attr.ib(default=attr.Factory(Experiments))
    test_ids = attr.ib(default=tuple())
    additional_options = attr.ib(default=attr.Factory(dict))
    request_id = attr.ib(default=None)
    rng_seed_salt = attr.ib(default=None)
    rng_seed = attr.ib(default=None)
    device_id = attr.ib(default=None)
    event = attr.ib(default=None, validator=_event_validator)
    voice_session = attr.ib(default=None)
    laas_region = attr.ib(default=None)
    device_state = attr.ib(default=attr.Factory(dict))
    dialog_id = attr.ib(default=None)
    prev_req_id = attr.ib(default=None)
    proxy_header = attr.ib(default={})
    sequence_number = attr.ib(default=None)
    session = attr.ib(default=None)
    srcrwr = attr.ib(default=attr.Factory(dict))
    ensure_purity = attr.ib(default=False)
    personal_data = attr.ib(default=None)
    data_sources = attr.ib(default=attr.Factory(dict))
    features = attr.ib(default=attr.Factory(dict))
    has_image_search_granet = attr.ib(default=False)
    request_label = attr.ib(default=None)
    rtlog_token = attr.ib(default=None)
    semantic_frames = attr.ib(default=attr.Factory(list))
    scenario_id = attr.ib(default=None)
    memento = attr.ib(default=None)

    def __attrs_post_init__(self):
        seed_str = ''
        if self.request_id is not None:
            seed_str += str(self.request_id)
        if self.rng_seed_salt is not None:
            seed_str += str(self.rng_seed_salt)

        if len(seed_str) > 0:
            object.__setattr__(self, 'rng_seed', md5.new(seed_str).hexdigest())

    def to_dict(self):
        res = attr.asdict(self)
        res['uuid'] = str(res['uuid'])
        res['callback_name'] = self.callback_name
        res['callback_args'] = self.callback_args
        res['experiments'] = self.experiments.to_dict()
        del res['event']
        del res['session']
        return res

    @property
    def callback_name(self):
        if isinstance(self.event, ServerActionEvent):
            return self.event.action_name
        else:
            return None

    @property
    def callback_args(self):
        if self.event is None:
            return None
        else:
            return self.event.payload

    def replace(self, **kwargs):
        return attr.evolve(self, **kwargs)


def create_request(uuid, utterance=None, client_time=None,
                   location=None, event=None, experiments=(), test_ids=(), device_id=None,
                   reset_session=False, request_id=None, app_info=None, device_state=None,
                   end_of_utterance=True, hypothesis_number=None,
                   **kwargs):
    """ Helper for create ReqInfo instance """
    utterance_ = utterance if utterance is None else Utterance(
        utterance,
        hypothesis_number=hypothesis_number,
        end_of_utterance=end_of_utterance,
    )

    return ReqInfo(
        uuid=uuid,
        request_id=request_id or str(uuid4()),
        device_id=device_id,
        utterance=utterance_,
        client_time=client_time or utcnow(),
        location=location,
        app_info=app_info or AppInfo(),
        event=event or TextInputEvent(utterance),
        experiments=Experiments(experiments),
        test_ids=test_ids,
        reset_session=reset_session,
        device_state=device_state or {},
        **kwargs
    )
