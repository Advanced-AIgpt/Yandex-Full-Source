# coding: utf-8

from __future__ import unicode_literals
import attr


@attr.s
class Intent(object):
    """
    Represents an NLU intent
    """
    name = attr.ib()
    parent_name = attr.ib(default=None)
    prior = attr.ib(default=1.0)
    reset_form = attr.ib(default=False)
    suggests = attr.ib(default=None)
    submit_form = attr.ib(default=None)
    allowed_prev_intents = attr.ib(default=None)
    allowed_device_states = attr.ib(default=None)
    allowed_apps = attr.ib(default=None)
    experiments = attr.ib(default=None)
    gc_fallback = attr.ib(default=False)
    allowed_active_slots = attr.ib(default=None)
    show_led_gif = attr.ib(default=None)
    gc_microintent = attr.ib(default=False)
    gc_swear_microintent = attr.ib(default=False)

    @classmethod
    def from_config(cls, intent_cfg):
        return cls(intent_cfg.name, intent_cfg.parent, intent_cfg.prior, intent_cfg.reset_form,
                   intent_cfg.suggests, intent_cfg.submit_form,
                   intent_cfg.allowed_prev_intents, intent_cfg.allowed_device_states, intent_cfg.allowed_apps,
                   intent_cfg.experiments, intent_cfg.gc_fallback, intent_cfg.allowed_active_slots, intent_cfg.show_led_gif, intent_cfg.gc_microintent, intent_cfg.gc_swear_microintent)

    def to_dict(self):
        # this method is used to save intent in a session - in this case, only intent name matters (for prev_intent)
        return {
            'name': self.name
        }

    @classmethod
    def from_dict(cls, value):
        return cls(value['name'])
