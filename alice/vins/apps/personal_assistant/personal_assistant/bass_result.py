# coding: utf-8

import logging
import attr

from personal_assistant.blocks import parse_block
from vins_core.utils.mixins import ToDictMixin


logger = logging.getLogger(__name__)


@attr.s
class BassSetupResult(object):
    forms = attr.ib(default=attr.Factory(list))

    @classmethod
    def from_dict(cls, data):
        forms = map(BassFormSetup.from_dict, data['forms'])
        for form in forms:
            if form:
                form.precomputed_data.update(data.get('shared_report_data', {}))
        return cls(
            forms=forms
        )


@attr.s
class BassReportResult(object):
    form_info = attr.ib()
    blocks = attr.ib()
    session_state = attr.ib()

    @classmethod
    def empty(cls):
        return cls(form_info=None, blocks=[], session_state=None)

    def to_dict(self):
        return {
            'form_info': self.form_info and self.form_info.to_dict(),
            'blocks': [b.to_dict() for b in self.blocks],
            'session_state': self.session_state
        }

    @classmethod
    def from_dict(cls, data):
        return cls(
            form_info=BassFormInfo.from_dict(data['form_info']),
            blocks=map(parse_block, data['blocks']),
            session_state=data.get('session_state')
        )


@attr.s
class BassFormSetupMeta(object):
    is_feasible = attr.ib(default=True)
    factors_data = attr.ib(default=None)

    @classmethod
    def from_dict(cls, data):
        return cls(is_feasible=data['is_feasible'],
                   factors_data=data.get('factors_data', None))


@attr.s
class BassFormSetup(object):
    meta = attr.ib(default=BassFormSetupMeta())
    info = attr.ib(default=None)
    precomputed_data = attr.ib(default={})

    @classmethod
    def from_dict(cls, data):
        body = data.get('report_data', None)
        if not body:
            return cls()
        form = body.pop('form')
        return cls(
            meta=BassFormSetupMeta.from_dict(data['setup_meta']),
            info=BassFormInfo.from_dict(form),
            precomputed_data=body
        )


class BassFormInfoDeserializationError(Exception):
    pass


@attr.s
class BassFormInfo(ToDictMixin):
    name = attr.ib(validator=attr.validators.instance_of(basestring))
    slots = attr.ib(default=attr.Factory(list), validator=attr.validators.instance_of(list))
    set_new_form = attr.ib(default=False)
    dont_resubmit = attr.ib(default=False)

    @classmethod
    def from_dict(cls, data):
        try:
            return cls(
                name=data.get('name', data.get('form')),
                slots=[SlotInfo.from_dict(slot) for slot in data.get('slots') or []],
                set_new_form=data.get('set_new_form', False),
                dont_resubmit=data.get('dont_resubmit', False)
            )

        except TypeError:
            logger.error('Wrong form: %s', str(data))
            raise BassFormInfoDeserializationError

    @classmethod
    def from_form(cls, form):
        return cls.from_dict(form.to_dict())


@attr.s
class SlotInfo(ToDictMixin):
    name = attr.ib(validator=attr.validators.instance_of(basestring))
    type = attr.ib(validator=attr.validators.instance_of(basestring))
    value = attr.ib()
    source_text = attr.ib()
    optional = attr.ib(attr.validators.optional(attr.validators.instance_of(bool)))

    @classmethod
    def from_dict(cls, data):
        try:
            if data.get('types'):
                type_ = data.get('value_type')
                if isinstance(type_, list):
                    type_ = type_[0] if len(type_) > 0 else None
                if type_ is None and isinstance(data['types'], list) and len(data['types']) > 0:
                    type_ = data['types'][0]
            else:
                type_ = data.get('type')

            return cls(
                name=data.get('name', data.get('slot')),
                type=type_,
                value=data.get('value'),
                source_text=data.get('source_text'),
                optional=data.get('optional'),
            )

        except TypeError:
            logger.error('Wrong slot: %s', str(data))
            raise BassFormInfoDeserializationError
