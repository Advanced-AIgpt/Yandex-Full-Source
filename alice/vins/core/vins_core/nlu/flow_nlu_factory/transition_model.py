# coding: utf-8
from __future__ import unicode_literals
from collections import defaultdict
import logging

from vins_core.utils.data import load_data_from_file


logger = logging.getLogger(__name__)


class TransitionModel(object):

    def __init__(self, **kwargs):
        pass

    def __call__(self, intent, session, req_info=None):
        raise NotImplementedError()


class MarkovTransitionModel(TransitionModel):
    """
    Markov transition model returns score over two consecutive time-steps
    """
    def __init__(self, normalize=False, **kwargs):
        super(MarkovTransitionModel, self).__init__(**kwargs)
        self._model = {}
        self._normalize = normalize

    def add_transition(self, prev_intent_name, intent_name, score=1):
        assert intent_name is not None and isinstance(intent_name, basestring)
        assert prev_intent_name is None or isinstance(prev_intent_name, basestring)
        self._model[prev_intent_name, intent_name] = score

    def build(self):
        if self._normalize:
            raise NotImplementedError()

    def get_static_score(self, current_intent, posible_intent, default=0):
        return self._model.get((current_intent, posible_intent), default)

    def __call__(self, intent_name, session, req_info=None):
        assert intent_name is not None and isinstance(intent_name, basestring)
        return self.get_static_score(self._get_prev_intent_name(session), intent_name, 0)

    @staticmethod
    def _get_prev_intent_name(session):
        return session.intent_name

    @property
    def model(self):
        return self._model

    def is_priority_boost(self, boost):
        return False


def create_parent_child_model(intents, **kwargs):
    m = MarkovTransitionModel(**kwargs)

    parent_childs = defaultdict(set)
    for intent in intents:
        if not intent.parent_name:
            m.add_transition(None, intent.name)

        for prev_intent in intents:
            if not intent.parent_name:
                m.add_transition(prev_intent.name, intent.name)
            else:
                parent_childs[intent.parent_name].add(intent.name)

    # parents define cliques in intent graph
    for parent_name, child_names in parent_childs.iteritems():
        for child_name_i in child_names:
            m.add_transition(parent_name, child_name_i)
            for child_name_j in child_names:
                m.add_transition(child_name_i, child_name_j)

    return m


def create_model_from_file(intents, **kwargs):
    """
    Loads the transition model from a file. The file must be a json or a yaml map mapping source to target to score.
    A score for every source-target combination must be provided.
    """
    m = MarkovTransitionModel(**kwargs)

    filename = kwargs.get('filename')
    if filename is None:
        raise ValueError('A filename containing transition probabilities for '
                         'the manual transition model must be specified')
    transition_probs = load_data_from_file(filename)

    # None -> intent
    if None not in transition_probs:
        raise ValueError('Cannot find transition probabilities for no intent case')
    for intent in intents:
        if intent.name not in transition_probs[None]:
            raise ValueError('Cannot find transition probability [None] -> [%s]' % intent.name)
        m.add_transition(None, intent.name, transition_probs[None][intent.name])

    # src_intent -> target_intent
    for src_intent in intents:
        if src_intent.name not in transition_probs:
            raise ValueError('Cannot find transition probabilities for intent %s' % src_intent.name)

        for target_intent in intents:
            if target_intent.name not in transition_probs[src_intent.name]:
                raise ValueError('Cannot find transition probability [%s] -> [%s]'
                                 % (src_intent.name, target_intent.name))
            m.add_transition(src_intent.name, target_intent.name, transition_probs[src_intent.name][target_intent.name])

    return m


def create_model_from_data(data, **kwargs):
    m = MarkovTransitionModel(**kwargs)
    for (prev_intent_name, intent_name), score in data.iteritems():
        m.add_transition(prev_intent_name, intent_name, score)
    return m


_transition_model_factories = {}


def register_transition_model(name, factory):
    _transition_model_factories[name] = factory


register_transition_model('parent_child', create_parent_child_model)
register_transition_model('from_file', create_model_from_file)
register_transition_model('from_data', create_model_from_data)


def create_transition_model(intents, model_name='parent_child', **kwargs):
    """
    :param intents: a list of Intent instances
    :param model_name: one of the following models
            - parent_child: a simple markov model where Intent.context (parent)
                            determines child's clique. If all contexts are empty,
                            this model actually has no any effect
            - from_file: read transition probabilities from a specified data file
            or any other model registered using register_transition_model function.
    :param kwargs:
    :return: TransitionModel instance
    """
    model_factory = _transition_model_factories.get(model_name)
    if model_factory is None:
        raise ValueError('Unknown transition model: %s' % model_name)

    model = model_factory(intents=intents, **kwargs)
    model.build()

    return model
