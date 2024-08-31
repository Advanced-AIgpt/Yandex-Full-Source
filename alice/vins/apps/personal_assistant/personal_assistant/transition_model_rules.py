# -*- coding: utf-8 -*-
import logging
import re
import six

logger = logging.getLogger(__name__)


class BaseRule(object):
    def __init__(self, boost, *args, **kwargs):
        self._boost = boost

    def get_boost(self, intent_name, session_intent_name, form):
        if isinstance(self._boost, (int, float)):
            return self._boost
        else:
            raise ValueError("Boost must be specified for top level rules")

    def check(self, intent_name, session_intent_name, form):
        raise NotImplementedError


class UnconditionalRule(BaseRule):
    def __init__(self, curr_intent, *args, **kwargs):
        super(UnconditionalRule, self).__init__(*args, **kwargs)
        self._curr_intent = curr_intent

    def check(self, intent_name, session_intent_name, form):
        return self._curr_intent == intent_name

    @classmethod
    def from_dict(cls, d):
        if d.get('type') == 'unconditional':
            return cls(d['curr_intent'], boost=d.get('boost'))
        raise ValueError("Wrong rule type '%s'" % d.get('type'))


class CheckPrevCurrIntentRule(BaseRule):
    def __init__(self, prev_intent, curr_intent, *args, **kwargs):
        super(CheckPrevCurrIntentRule, self).__init__(*args, **kwargs)
        self._prev_intent = prev_intent
        self._curr_intent = curr_intent

    def _equal(self, intent_pattern, intent_name):
        # any intent
        if intent_pattern == '':
            return True
        return intent_pattern == intent_name

    def check(self, intent_name, session_intent_name, form):
        """
        compare current and previous intent in transition with intents specified in rule,
        if compare success transition will be boosted
        """
        if self._equal(self._curr_intent, intent_name):
            if isinstance(self._prev_intent, list):
                for prev_intent in self._prev_intent:
                    if self._equal(prev_intent, session_intent_name):
                        return True
                return False
            else:
                return self._equal(self._prev_intent, session_intent_name)
        return False

    @classmethod
    def from_dict(cls, d):
        if d.get('type') == 'check_prev_curr_intent':
            return cls(d['prev_intent'], d['curr_intent'], boost=d.get('boost'), use_regex=d.get('use_regex', False))
        raise ValueError("Wrong rule type '%s'" % d.get('type'))


class AllowPrevIntentsRule(BaseRule):
    def __init__(self, prev_intents, curr_intent, *args, **kwargs):
        super(AllowPrevIntentsRule, self).__init__(*args, **kwargs)
        self._prev_intents = set(prev_intents)
        self._curr_intent = curr_intent

    def get_boost(self, intent_name, session_intent_name, form):
        if session_intent_name not in self._prev_intents:
            return 0
        return super(AllowPrevIntentsRule, self).get_boost(intent_name, session_intent_name, form)

    def check(self, intent_name, session_intent_name, form):
        return intent_name == self._curr_intent

    @classmethod
    def from_dict(cls, d):
        if d.get('type') == 'allow_prev_intents':
            return cls(d['prev_intents'], d['curr_intent'], boost=d.get('boost'))
        raise ValueError("Wrong rule type '%s'" % d.get('type'))


class CheckFormActiveSlots(BaseRule):
    def __init__(self, prev_intent, curr_intent, *args, **kwargs):
        super(CheckFormActiveSlots, self).__init__(*args, **kwargs)
        self._prev_intent = prev_intent
        self._curr_intent = curr_intent

    def check(self, intent_name, session_intent_name, form):
        """
        if form have active slots boost transition prev_intent -> curr_intent
        """
        if form and form.has_active_slots():
            return self._prev_intent == session_intent_name and self._curr_intent == intent_name
        return False

    @classmethod
    def from_dict(cls, d):
        if d.get('type') == 'check_form_active_slots':
            return cls(d['prev_intent'], d['curr_intent'], boost=d.get('boost'))
        raise ValueError("Wrong rule type '%s'" % d.get('type'))


class CheckFormSlotValueRule(BaseRule):
    def __init__(self, slot, value, slot_value_key=None, *args, **kwargs):
        super(CheckFormSlotValueRule, self).__init__(*args, **kwargs)
        self._slot = slot
        self._slot_value_key = slot_value_key
        self._value = value

    def check(self, intent_name, session_intent_name, form):
        """
        check conditions of the rule are satisfied
        will find special slot in current form and then compare it\'s value
        """
        if not form:
            return False

        form_slot = form.get_slot_by_name(self._slot)
        if not form_slot:
            return False
        val = form_slot.value
        if self._slot_value_key is not None:
            if not isinstance(val, dict) or not val:
                return False
            return val.get(self._slot_value_key) == self._value
        if isinstance(val, six.string_types) and isinstance(self._value, six.string_types):
            return re.match(self._value, val)
        return val == self._value

    @classmethod
    def from_dict(cls, d):
        if d.get('type') == 'check_form_slot_value':
            return cls(d['slot'], d['value'], d.get('slot_value_key'), boost=d.get('boost'))
        raise ValueError("Wrong rule type '%s'" % d.get('type'))


_operations = {
    'and': (lambda x, y: x and y),
    'or': (lambda x, y: x or y),
}


class LogicRule(BaseRule):
    def __init__(self, operation, children, *args, **kwargs):
        super(LogicRule, self).__init__(*args, **kwargs)
        self._op = _operations.get(operation)
        self._op_str = operation
        if not self._op:
            raise ValueError("Can't find logic rule operand")
        self._children = []
        for child in children:
            self._children.extend(create_transition_rules([child]))
        if len(self._children) < 2:
            raise ValueError("Logic rule must have two or more children rules")

    def check(self, intent_name, session_intent_name, form):
        """
        logic rule checks result of or/and operation over all
        children rules
        """
        res = self._children[0].check(intent_name, session_intent_name, form)
        for child in self._children[1:]:
            # We can finish calculating the rule in advance
            if res and self._op_str == 'and' or not res and self._op_str == 'or':
                res = self._op(res, child.check(intent_name, session_intent_name, form))
            else:
                return res
        return res

    @classmethod
    def from_dict(cls, d):
        if d.get('type') == 'logic_rule':
            return cls(d['operation'], d['children'], boost=d.get('boost'))
        raise ValueError("Wrong rule type '%s'" % d.get('type'))


RULE_TYPES = {
    'check_form_slot_value': CheckFormSlotValueRule,
    'check_prev_curr_intent': CheckPrevCurrIntentRule,
    'logic_rule': LogicRule,
    'check_form_active_slots': CheckFormActiveSlots,
    'allow_prev_intents': AllowPrevIntentsRule,
    'unconditional': UnconditionalRule,
}


def create_transition_rules(rules):
    all_rules = []
    for rule in rules:
        rule_type = RULE_TYPES.get(rule['type'])
        if rule_type:
            all_rules.append(rule_type.from_dict(rule))
        else:
            raise ValueError("Unknown rule type")
    return all_rules
