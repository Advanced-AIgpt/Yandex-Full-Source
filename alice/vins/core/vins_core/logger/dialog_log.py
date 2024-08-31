# coding: utf-8
from __future__ import unicode_literals

import logging

from datetime import datetime
from itertools import ifilter
from jsonschema import validate
from uuid import UUID

from vins_core.utils.config import get_bool_setting
from vins_core.utils.data import delete_keys_from_dict
from vins_core.utils.datetime import datetime_to_timestamp, timestamp_in_ms, utcnow, get_tz_name
from vins_core.utils.logging import REQUEST_FIELDS_BLACKLIST


dialog_log = logging.getLogger('dialog_history')

LOG_SCHEMA = {
    '$schema': 'http://json-schema.org/draft-04/schema#',
    'type': 'object',
    'definitions': {},
    'properties': {
        'app_id': {'type': 'string'},
        'client_time': {'type': 'integer'},
        'client_tz': {'type': 'string'},
        'request': {'type': ['object']},
        'response': {
            'type': 'object',
            'properties': {
                'voice_text': {'type': ['string', 'null']},
                'cards': {'type': 'array', 'items': {'type': 'object'}},
                'directives': {'type': 'array', 'items': {'type': 'object'}},
                'suggests': {'type': 'array', 'items': {'type': 'object'}},
                'should_listen': {'type': ['boolean', 'null']},
            },
            'additionalProperties': True,
        },
        'form': {
            'type': ['object', 'null'],
            'properties': {
                'form': {'type': 'string'},
                'slots': {'type': 'array'},
            },
        },
        'form_name': {'type': ['string', 'null']},
        'provider': {'type': 'string'},
        'type': {'type': 'string'},
        'server_time': {'type': 'integer'},
        'server_time_ms': {'type': 'integer'},
        'utterance_source': {'type': ['string', 'null']},
        'utterance_text': {'type': ['string', 'null']},
        'uuid': {'type': 'string'},
        'callback_name': {'type': ['string', 'null']},
        'callback_args': {'type': ['object', 'null']},
        'location_lat': {'type': ['number', 'null']},
        'location_lon': {'type': ['number', 'null']},
        'lang': {'type': ['string', 'null']},
        'experiments': {'type': 'object'},
        'device_id': {'type': ['string', 'null']},
        'session': {'type': ['string', 'null']},
    },
    'additionalProperties': False,
    'required': [
        'app_id', 'client_time', 'request', 'response', 'form',
        'form_name', 'provider', 'type', 'server_time', 'server_time_ms',
        'utterance_source', 'utterance_text', 'uuid', 'callback_name',
        'callback_args', 'location_lat', 'location_lon',
        'lang', 'experiments',
    ]
}


def _json_default(obj):
    if isinstance(obj, datetime):
        obj = datetime_to_timestamp(obj)
    elif isinstance(obj, UUID):
        return str(obj)
    return obj


def get_sensitive_meta(log_entry):
    if 'meta' not in log_entry['response']:
        return None
    for meta in log_entry['response'].get('meta', []):
        if meta['type'] == 'sensitive':
            return meta
    return None


def wipe_out_slot(slot):
    for field in ('value', 'source_text'):
        if field in slot and slot[field] is not None:
            slot[field] = '***'


def get_field(log_entry, field_path):
    current = log_entry
    parts = field_path.split('.')
    for part in parts:
        if current.get(part) is not None:
            current = current[part]
        else:
            return None
    return current


def wipe_out_field(log_entry, field, placeholder):
    current = log_entry
    parts = field.split('.')
    path_parts = parts[:-1]
    field_name = parts[-1]
    for part in path_parts:
        if current.get(part) is not None:
            current = current[part]
        else:
            return
    if current.get(field_name) is not None:
        current[field_name] = placeholder


def wipe_out(log_entry, fields, placeholder='***'):
    for field in fields:
        wipe_out_field(log_entry, field, placeholder)


def wipe_out_sensitive_content(log_entry, sensitive_meta):
    sensitive_slots = set(get_field(sensitive_meta, 'data.slots'))
    slot_paths = ('form.slots', 'callback_args.form_update.slots')
    if sensitive_slots is not None:
        for slot in log_entry['form']['slots']:
            if slot['slot'] in sensitive_slots:
                wipe_out_slot(slot)

        form_update_slots = get_field(log_entry, 'callback_args.form_update.slots')
        if form_update_slots is not None:
            for slot in form_update_slots:
                if slot.get('name') in sensitive_slots:
                    wipe_out_slot(slot)
    else:
        wipe_out(log_entry, slot_paths, ['***'])

    wipe_out(
        log_entry,
        ('request.utterance.text',
         'utterance_text',
         'response.voice_text')
    )

    wipe_out(
        log_entry,
        ('response.suggests',
         'response.directives',
         'response.cards'),
        [{'***': '***'}]
    )

    return log_entry


def make_log_entry(session, request, response):
    nowutc = utcnow()
    res = {
        'app_id': session.app_id,
        'client_time': datetime_to_timestamp(request.client_time),
        'client_tz': get_tz_name(request.client_time),
        'request': request.to_dict(),
        'response': response.to_dict(),
        # form
        # form name
        'provider': 'vins',
        # type
        'server_time': datetime_to_timestamp(nowutc),
        'server_time_ms': timestamp_in_ms(nowutc),
        # utterance_source
        # utterance_text
        # uuid
        'callback_name': request.callback_name,
        'callback_args': request.callback_args,
        # location_lat
        # location_lon
        'lang': request.lang,
        'experiments': request.experiments.to_dict(),
        'device_id': request.device_id and str(request.device_id),
        'session': request.session and str(request.session),
    }

    # form
    form = session.form and session.form.to_dict(truncate=True)
    res['form'] = form
    res['form_name'] = (form or {}).get('form')

    # type
    if request.callback_name is not None:
        res['type'] = 'CALLBACK'
    elif request.utterance is not None:
        res['type'] = 'UTTERANCE'
    else:
        raise ValueError('Unknown type of dialog request')

    # utterance
    utt = (request.utterance and request.utterance.to_dict()) or {}
    res['utterance_source'] = utt.get('input_source')
    res['utterance_text'] = utt.get('text')

    # uuid
    uuid = request.uuid
    if isinstance(uuid, UUID):
        res['uuid'] = uuid.hex
    else:
        res['uuid'] = str(uuid)

    # location
    location = request.location or {}
    res['location_lat'] = location.get('lat')
    res['location_lon'] = location.get('lon')

    # TODO rethink sensetive content wiping
    # sensitive_meta = get_sensitive_meta(res)
    # if sensitive_meta is not None:
    #     wipe_out_sensitive_content(res, sensitive_meta)

    disable_json_validation = get_bool_setting('DISABLE_JSON_VALIDATION', default=False)
    if not disable_json_validation:
        validate(res, LOG_SCHEMA)

    return res


def _get_user_id_cookie(data):
    return next(ifilter(
        lambda cookie: cookie.startswith('user_id='),
        data.get('request', {}).get('additional_options', {}).get('bass_options', {}).get('cookies') or []
    ), None)


def _restore_user_id_cookie(data, user_id):
    if not user_id:
        return
    if ('request' not in data or
            'additional_options' not in data['request'] or
            'bass_options' not in data['request']['additional_options']):
        return
    data['request']['additional_options']['bass_options']['cookies'] = [user_id]


def _create_dialog_log_entry(session, request, response):
    data = make_log_entry(session, request, response)
    user_id_cookie = _get_user_id_cookie(data)
    data = delete_keys_from_dict(data, REQUEST_FIELDS_BLACKLIST)
    _restore_user_id_cookie(data, user_id_cookie)

    return data
