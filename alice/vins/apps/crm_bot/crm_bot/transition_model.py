# coding: utf-8
from __future__ import unicode_literals

from copy import copy

from personal_assistant.transition_model import PersonalAssistantTransitionModel
from crm_bot.intents import FEEDBACK_WHITELIST, VERBOSE_REDIRECT_INTENTS


class CrmBotTransitionModel(PersonalAssistantTransitionModel):
    def __call__(self, intent_name, session, req_info=None):
        if req_info is not None and req_info.experiments['disable__'+intent_name.split('.')[-1]] is not None:
            return 0
        return super(CrmBotTransitionModel, self).__call__(intent_name, session, req_info)

    def _parse_transition_rules(self, custom_rules):
        rules = super(CrmBotTransitionModel, self)._parse_transition_rules(custom_rules=custom_rules)

        # feedback intents activation boosts
        rule_feedback = {
            "type": "allow_prev_intents",
            "prev_intents": FEEDBACK_WHITELIST,
            "curr_intent": None,
            "boost": (1.0 + self.boosts.internal) * 0.5  # half the internal intent boost
        }

        rule_feedback.update({'curr_intent': "crm_bot.scenarios.feedback_positive"})
        rules.append(copy(rule_feedback))

        rule_feedback.update({'curr_intent': "crm_bot.scenarios.feedback_negative"})
        rules.append(copy(rule_feedback))

        # verbose redirect boosts
        rule_redirect = {
            "type": "allow_prev_intents",
            "prev_intents": VERBOSE_REDIRECT_INTENTS,
            "curr_intent": None,
            "boost": (1.0 + self.boosts.internal) * 0.5  # half the internal intent boost
        }

        rule_redirect.update({'curr_intent': "crm_bot.scenarios.operator_redirect_verbose_continuation"})
        rules.append(copy(rule_redirect))

        return rules


def create_crmbot_transition_model(*args, **kwargs):
    return CrmBotTransitionModel(*args, **kwargs)
