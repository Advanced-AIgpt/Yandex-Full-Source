from __future__ import unicode_literals

import logging

from enum import Enum
from itertools import ifilter

from vins_core.utils.operators import attr
from vins_core.utils.strings import smart_unicode
from .events import (
    NLGEventHandler, SayEventHandler,
    CallbackEventHandler, GeneralConversationEventHandler,
    ClearSlotEventHandler, ChangeSlotPropertiesEventHandler, RestorePrevFormEventHandler,
    ClearSlotsIfAnyUpdatedEventHandler
)


logger = logging.getLogger(__name__)


class SlotConcatenation(Enum):
    """
    Two consecutive occurrences X1 and X2 of the same string-valued slot X are processed depending on this setting:
    - allow - they are concatenated into as single string, if and only if X2 starts with an I-tag ("+" in the markup)
    - force - they are always concatenated
    - forbid - they are never concatenated
    """
    allow = 1
    force = 2
    forbid = 3


class Handler(object):
    @classmethod
    def from_dict(cls, obj):
        if obj:
            return cls(**obj)

    def __init__(self, **kw):
        self.handler_name = kw.pop('handler', None) or kw.pop('name')
        self.kw = kw
        if self.handler_name == 'nlg':
            self._handler = NLGEventHandler(**kw)
        elif self.handler_name == 'say':
            self._handler = SayEventHandler(**kw)
        elif self.handler_name == 'callback':
            self._handler = CallbackEventHandler(**kw)
        elif self.handler_name == 'general_conversation':
            self._handler = GeneralConversationEventHandler(**kw)
        elif self.handler_name == 'clear_slot':
            self._handler = ClearSlotEventHandler(**kw)
        elif self.handler_name == 'clear_slots_if_any_updated':
            self._handler = ClearSlotsIfAnyUpdatedEventHandler(**kw)
        elif self.handler_name == 'change_slot_properties':
            self._handler = ChangeSlotPropertiesEventHandler(**kw)
        elif self.handler_name == 'restore_prev_form':
            self._handler = RestorePrevFormEventHandler(**kw)
        else:
            raise ValueError('Unknown handler name: %s' % self.handler_name)

    def to_dict(self):
        obj = {
            'handler': self.handler_name,
        }
        obj.update(self.kw)
        return obj

    def handle(self, *args, **kwargs):
        kwargs.update(self.kw)
        logger.info('Calling event handler "{}"'.format(self.handler_name))
        return self._handler.handle(*args, **kwargs)


class Event(object):
    @classmethod
    def from_dict(cls, obj):
        handlers = obj.get('handlers') or []
        return cls(
            obj.get('name') or obj.get('event'),
            handlers=[Handler.from_dict(_) for _ in handlers],
        )

    def __init__(self, name, handlers=None):
        self.name = name
        self.handlers = handlers or []

    def to_dict(self):
        return {
            'event': self.name,
            'handlers': [_.to_dict() for _ in self.handlers],
        }

    def handle(self, form, dm, sample, session, app, req_info, response, **kw):
        for handler in self.handlers:
            handler.handle(
                form=form, dm=dm, sample=sample, session=session,
                app=app, req_info=req_info, response=response, **kw
            )


class Slot(object):
    @classmethod
    def from_dict(cls, obj):
        return cls(
            name=obj.get('name') or obj.get('slot'),
            types=obj.get('types') or [obj['type']],
            optional=obj['optional'],
            events=[Event.from_dict(_) for _ in obj.get('events') or []],
            active=obj.get('active', False),
            value=obj.get('value'),
            source_text=obj.get('source_text'),
            matching_type=obj.get('matching_type'),
            share_tags=obj.get('share_tags'),
            import_tags=obj.get('import_tags'),
            import_entity_types=obj.get('import_entity_types'),
            import_entity_tags=obj.get('import_entity_tags'),
            import_entity_pronouns=obj.get('import_entity_pronouns'),
            expected_values=obj.get('expected_values'),
            disabled=obj.get('disabled', False),
            value_type=obj.get('value_type', None),
            normalize_to=obj.get('normalize_to', None),
            prefix_normalization=obj.get('prefix_normalization', None),
            allow_multiple=obj.get('allow_multiple', False),
            source_annotation=obj.get('source_annotation', None),
            concatenation=SlotConcatenation[obj.get('concatenation', SlotConcatenation.forbid.name)]
        )

    def __init__(self, name, types, optional=False, events=None,
                 active=False, value=None, source_text=None, matching_type=None, share_tags=None,
                 expected_values=None, import_tags=None, import_entity_tags=None, import_entity_types=None,
                 import_entity_pronouns=None, disabled=False, value_type=None, normalize_to=None,
                 prefix_normalization=None, allow_multiple=False, source_annotation=None,
                 concatenation=SlotConcatenation.forbid):
        self.name = smart_unicode(name)
        self.types = types
        self.optional = optional
        self._events = events or []
        self.active = active
        self._value = value
        self._source_text = source_text
        self._value_type = value_type
        self.matching_type = matching_type or 'exact'
        self._share_tags = frozenset(share_tags or [])
        self._expected_values = expected_values
        self._import_tags = frozenset(import_tags or [])
        self._import_entity_types = frozenset(import_entity_types or [])
        self._import_entity_tags = frozenset(import_entity_tags or [])
        self._import_entity_pronouns = frozenset(import_entity_pronouns or [])
        self.disabled = disabled
        self.normalize_to = normalize_to
        self.prefix_normalization = prefix_normalization
        self._allow_multiple = allow_multiple
        self.source_annotation = source_annotation
        assert concatenation in SlotConcatenation, \
            "Slot concatenation setting must be one of the vins_core.dm.form_filler.models.SlotConcatenation members"
        self._concatenation = concatenation

        if (self.import_entity_tags or self.import_entity_types) and 'string' not in self.types:
            raise RuntimeError('import_entity_tags/import_entity_types are currently supported only '
                               'for slots with possible "string" type. Please, check slot "%s"', self.name)
        if (self.import_entity_tags or self.import_entity_types) and not self.import_entity_pronouns:
            raise RuntimeError('"import_entity_pronouns" must be specified if using '
                               'import_entity_tags/import_entity_types. Please, check slot "%s"', self.name)

    @property
    def value(self):
        return self._value

    @property
    def source_text(self):
        return self._source_text

    @property
    def value_type(self):
        return self._value_type

    @property
    def expected_values(self):
        return self._expected_values

    @expected_values.setter
    def expected_values(self, value):
        self._expected_values = list(value) if value else None

    @property
    def share_tags(self):
        return self._share_tags

    @property
    def import_tags(self):
        return self._import_tags

    @property
    def import_entity_tags(self):
        return self._import_entity_tags

    @property
    def import_entity_types(self):
        return self._import_entity_types

    @property
    def import_entity_pronouns(self):
        return self._import_entity_pronouns

    @property
    def allow_multiple(self):
        return self._allow_multiple

    @property
    def concatenation(self):
        return self._concatenation

    @property
    def events(self):
        return self._events

    def to_dict(self, truncate=False):
        js = {
            'slot': self.name,
            'optional': self.optional,
            'active': self.active,
            'types': self.types,
            'events': [_.to_dict() for _ in self._events],
            'value': self._value,
            'source_text': self._source_text,
            'value_type': self._value_type,
            'matching_type': self.matching_type,
            'share_tags': list(self.share_tags),
            'expected_values': self._expected_values,
            'import_tags': list(self.import_tags),
            'import_entity_types': list(self.import_entity_types),
            'import_entity_tags': list(self.import_entity_tags),
            'import_entity_pronouns': list(self.import_entity_pronouns),
            'disabled': self.disabled,
            'normalize_to': self.normalize_to,
            'allow_multiple': self._allow_multiple,
            'concatenation': self._concatenation.name
        }

        if truncate:
            del js['events']

        return js

    def raise_event(self, event_name, form, dm, sample, session,
                    app, req_info, required_handler, response, **kw):
        logger.debug("Raise event '%s' for '%s.%s'", event_name, form.name, self.name)

        handlers = self.get_event_handlers(event_name)
        if required_handler and not handlers:
            raise RuntimeError(
                "'%s' event handler is needed for '%s.%s'" %
                (event_name, form.name, self.name)
            )

        for handler in handlers:
            handler.handle(
                form=form, dm=dm, sample=sample, session=session,
                app=app, req_info=req_info,
                response=response, **kw)

    def get_event_handlers(self, event_name):
        return filter(attr('name') == event_name, self.events)

    def set_value(self, value, value_type, source_text=None):
        self._value = value
        self._value_type = value_type
        self._source_text = source_text

    def reset_value(self):
        self._value = None
        self._value_type = None
        self._source_text = None


class RequiredSlotGroup(object):
    @classmethod
    def from_dict(cls, obj):
        return cls(
            slots=obj['slots'],
            slot_to_ask=obj['slot_to_ask'],
        )

    def __init__(self, slots, slot_to_ask):
        assert slots, "A required slot group must have a non-empty slot list"
        assert slot_to_ask, "The name of a slot to ask must be specified for a required slot group"

        self.slots = list(slots)
        self.slot_to_ask = slot_to_ask

    def to_dict(self):
        return {
            'slots': self.slots,
            'slot_to_ask': self.slot_to_ask,
        }


class Form(object):
    @classmethod
    def from_dict(cls, obj):
        slots = obj.get('slots', [])
        events = obj.get('events', [])
        required_slot_groups = obj.get('required_slot_groups', [])
        return cls(
            obj.get('name') or obj.get('form'),
            slots=[Slot.from_dict(s) for s in slots],
            events=[Event.from_dict(e) for e in events],
            required_slot_groups=[
                RequiredSlotGroup.from_dict(rsg) for rsg in required_slot_groups
            ],
            is_overridden=obj.get('is_overridden') or False,
            data_sources=obj.get('data_sources')
        )

    def __init__(self, name, slots=None, events=None, required_slot_groups=None, is_overridden=False,
                 data_sources=None):
        assert name is not None

        self.name = name
        self._slots = slots or []
        self._slots_map = {s.name: s for s in slots or []}
        self._data_sources = data_sources or {}
        self._events = events or []
        self._required_slot_groups = required_slot_groups or []
        self.is_ellipsis = False
        self.shares_slots_with_previous_form = False
        self.is_overridden = is_overridden

        # Check if all slots in slot groups are valid
        assert all(self.get_slot_by_name(g.slot_to_ask) for g in self._required_slot_groups)
        assert all(self.get_slot_by_name(s) for g in self._required_slot_groups for s in g.slots)
        assert all(g.slot_to_ask in g.slots for g in self._required_slot_groups)

    def __unicode__(self):
        return 'Form({0})'.format(', '.join([self.name] + ['{0}={1}'.format(s.name, s.value) for s in self._slots]))

    def __getattribute__(self, name):
        try:
            return super(Form, self).__getattribute__(name)
        except AttributeError as e:
            if name != '_slots_map' and name in self._slots_map:
                return self._slots_map[name]
            else:
                raise e

    def __contains__(self, obj):
        return obj in self._slots_map

    def __getitem__(self, key):
        return self._slots_map[key].value

    def __setitem__(self, key, value):
        self._slots_map[key].value = value

    def to_dict(self, truncate=False):
        js = {
            'form': self.name,
            'slots': [s.to_dict(truncate) for s in self._slots],
            'events': [e.to_dict() for e in self._events],
            'required_slot_groups': [rsg.to_dict() for rsg in self._required_slot_groups],
            'is_ellipsis': self.is_ellipsis,
            'shares_slots_with_previous_form': self.shares_slots_with_previous_form,
        }

        if truncate:
            del js['events']
            del js['required_slot_groups']

        if self.is_overridden:
            js['is_overridden'] = True

        if self._data_sources:
            js['data_sources'] = self._data_sources

        return js

    @property
    def events(self):
        return self._events

    @property
    def slots(self):
        return self._slots

    @property
    def required_slot_groups(self):
        return self._required_slot_groups

    def get_slot_by_name(self, slot_name):
        return self._slots_map.get(slot_name)

    def raise_event(self, event_name, dm, sample, sample_features, session, app,
                    req_info, required_handler, response, **kw):
        logger.debug("Raise event '%s' for '%s'", event_name, self.name)

        handlers = self.get_event_handlers(event_name)
        if required_handler and not handlers:
            raise RuntimeError(
                "'%s' event handler is needed for '%s'" %
                (event_name, self.name)
            )

        for handler in handlers:
            handler.handle(
                form=self, dm=dm, session=session, sample=sample,
                sample_features=sample_features, app=app, req_info=req_info,
                response=response, **kw
            )

    def get_event_handlers(self, event_name):
        return filter(attr('name') == event_name, self.events)

    def has_active_slots(self):
        for s in self.active_slots():
            return True
        return False

    def has_expected_values(self):
        for s in self.active_slots():
            if s.expected_values:
                return True
        return False

    def get_expected_values(self):
        for s in self.active_slots():
            if s.expected_values:
                return s.expected_values
        return False

    def active_slots(self):
        return ifilter(attr('active') == True, self.slots)  # noqa

    def disabled_slots(self):
        return ifilter(attr('disabled') == True, self.slots)  # noqa

    def is_continuing(self):
        # fast fix for vins.get_news IsContinuing PASKILLS-5380
        if self.name == 'personal_assistant.scenarios.get_news':
            return False
        # switching to protocol scenario DIALOG-7241
        if self.name == 'personal_assistant.scenarios.search':
            return False
        # fast fix for skills_discovery IsContinuing for ALICE-9433
        if self.name == 'personal_assistant.scenarios.search__skills_discovery':
            return False
        return self.is_ellipsis or self.shares_slots_with_previous_form
