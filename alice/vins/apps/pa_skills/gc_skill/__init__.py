# coding: utf-8

from vins_core.nlu.flow_nlu_factory.transition_model import MarkovTransitionModel, register_transition_model
from personal_assistant import intents


class GCSkillTransitionModel(MarkovTransitionModel):
    def __init__(self, intents, disable_intents=None, **kwargs):
        super(GCSkillTransitionModel, self).__init__(**kwargs)
        self._disabled = set(disable_intents or ())
        self._allowed_prev_intents = {intent.name: intent.allowed_prev_intents for intent in intents}

    def __call__(self, intent_name, session, req_info=None):
        if intent_name in self._disabled:
            return 0.0

        if intents.is_microintent(intent_name):
            allowed_prev_intents = self._allowed_prev_intents.get(intent_name)

            if allowed_prev_intents:
                if not session.intent_name:
                    return 0.0
                return float(intents.trim_name_prefix(session.intent_name, 1) in allowed_prev_intents)

        return 1.0


register_transition_model('gc_skill', GCSkillTransitionModel)
