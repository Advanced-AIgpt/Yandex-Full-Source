# coding: utf-8
from __future__ import unicode_literals

import attr

from vins_core.dm.response import Meta


@attr.s
class AttentionMeta(Meta):
    attention_type = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'attention'

    @classmethod
    def from_dict(cls, data):
        return cls(attention_type=data['attention_type'])


@attr.s
class CancelListening(Meta):
    form_name = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'cancel_listening'

    @classmethod
    def from_dict(cls, data):
        return cls(form_name=data['form_name'])


@attr.s
class RepeatMeta(Meta):
    def __attrs_post_init__(self):
        self.type = 'repeat'

    @classmethod
    def from_dict(cls, data):
        return cls()


@attr.s
class ExternalSkillMeta(Meta):
    deactivating = attr.ib(default=False, type=bool)
    skill_name = attr.ib(default=None, type=str)

    def __attrs_post_init__(self):
        self.type = 'external_skill'

    @classmethod
    def from_dict(cls, data):
        return cls(
            deactivating=data['deactivating'],
            skill_name=data.get('skill_name')
        )


@attr.s
class DebugInfoMeta(Meta):
    data = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'debug_info'

    @classmethod
    def from_dict(cls, data):
        return cls(data=data['data'])


@attr.s
class GeneralConversationMeta(Meta):
    pure_gc = attr.ib(default=False)

    def __attrs_post_init__(self):
        self.type = 'general_conversation'

    @classmethod
    def from_dict(cls, data):
        return cls(pure_gc=data['pure_gc'])


@attr.s
class GeneralConversationSourceMeta(Meta):
    source = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'gc_source'

    @classmethod
    def from_dict(cls, data):
        return cls(source=data['source'])


@attr.s
class SensitiveMeta(Meta):
    data = attr.ib(default=None)

    def __attrs_post_init__(self):
        self.type = 'sensitive'

    @classmethod
    def from_dict(cls, data):
        return cls(data=data['data'])
