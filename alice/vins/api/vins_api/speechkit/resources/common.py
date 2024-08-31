# coding: utf-8

from datetime import datetime
from uuid import UUID

import falcon
import re

from alice.vins.api_helper.resources import ValidationError, parse_json_request
from vins_core.dm.request import AppInfo, ReqInfo, configure_experiment_flags, Experiments
from vins_core.dm.response import ErrorMeta
from vins_core.dm.request_events import RequestEvent
from vins_core.utils.datetime import parse_tz
from vins_core.utils.metrics import sensors


OPTIONS_FROM_HEADERS = {'x-yandex-joker-test': 'joker', 'x-yandex-via-joker': 'joker_proxy'}

SOMETHING_WRONG_HTTP_STATUS_CODE = '512'
EOU_EXPECTED_CODE = falcon.HTTP_206
CANCEL_LISTENING_CODE = falcon.HTTP_205
SENSITIVE_META_ERROR_TYPES = [
    'bass_error',
]


def get_device_id(device_id):
    uuid_pattern = r'^\{?[0-9a-fA-F]{8}\-?([0-9a-fA-F]{4}\-?){3}[0-9a-fA-F]{12}\}?$'
    if re.match(uuid_pattern, device_id):
        return UUID(device_id).hex
    return device_id


def get_request_event(data):
    return RequestEvent.from_dict(data['request']['event'])


def get_client_time(user_info):
    return client_time_from_timestamp(float(user_info['timestamp']), tz=user_info['timezone'])


def client_time_from_timestamp(timestamp, tz):
    return datetime.fromtimestamp(timestamp, tz=parse_tz(tz))


def parse_srcrwr(srcrwr_data):
    srcrwr = {}
    if srcrwr_data:
        for single_header in srcrwr_data.split(','):
            for part in single_header.split(';'):
                src_name, src_url = part.split('=', 1)
                srcrwr[src_name] = src_url

    return srcrwr


def get_req_info(event, req_data, experiments, http_request=None, rng_seed_salt=None):
    application_data = req_data['application']
    request_experiments = req_data['request'].get('experiments', {})
    experiments = configure_experiment_flags(experiments, request_experiments)
    srcrwr_data = http_request.get_header('x-srcrwr') if http_request else None
    srcrwr = parse_srcrwr(srcrwr_data)
    additional_options = req_data['request'].get('additional_options', {})
    proxy_header = {}
    if http_request:
        if hasattr(http_request, 'headers'):
            prefix = 'x-yandex-proxy-header-'
            for header, value in http_request.headers.items():
                if header.lower().startswith(prefix):
                    proxy_header[header[len(prefix):]] = value

        for header_name, option_name in OPTIONS_FROM_HEADERS.iteritems():
            header_value = http_request.get_header(header_name, default='')
            header_value.strip()
            if header_value:
                additional_options[option_name] = header_value

    return ReqInfo(
        uuid=UUID(application_data['uuid']),
        device_id=get_device_id(application_data.get('device_id', '')),
        client_time=get_client_time(application_data),
        utterance=event.utterance,
        lang=application_data['lang'],
        location=req_data['request'].get('location'),
        reset_session=req_data['request'].get('reset_session', False),
        experiments=Experiments(experiments),
        test_ids=req_data['request'].get('test_ids', []),
        dialog_id=req_data['header'].get('dialog_id'),
        request_id=req_data['header'].get('request_id'),
        rng_seed_salt=rng_seed_salt,
        prev_req_id=req_data['header'].get('prev_req_id'),
        sequence_number=req_data['header'].get('sequence_number'),
        app_info=AppInfo(
            platform=application_data['platform'],
            os_version=application_data['os_version'],
            app_id=application_data['app_id'],
            app_version=application_data['app_version'],
            device_manufacturer=application_data.get('device_manufacturer'),
            device_model=application_data.get('device_model'),
        ),
        voice_session=req_data['request'].get('voice_session'),
        additional_options=additional_options,
        event=event,
        laas_region=req_data['request'].get('laas_region'),
        device_state=req_data['request'].get('device_state'),
        proxy_header=proxy_header,
        session=req_data.get('session'),
        srcrwr=srcrwr,
        ensure_purity=req_data['request'].get('ensure_purity'),
        personal_data=req_data['request'].get('personal_data'),
        features=req_data['request'].get('features'),
        has_image_search_granet=req_data['request'].get('has_image_search_granet', False)
    )


def parse_sk_request(http_request, json_schema, experiments, rng_seed_salt=None):
    with sensors.timer('view_parse_time'):
        req_data = parse_json_request(http_request, json_schema)
        try:
            event = get_request_event(req_data)
        except ValueError:
            raise ValidationError(
                'Unsupported event type "%r" in request' % req_data.get('request', {}).get('event')
            )
        except Exception as e:
            raise ValidationError(
                'Invalid event params, inner exception: "%s"' % e
            )
        req_info = get_req_info(event, req_data, experiments, http_request, rng_seed_salt)
    return req_info, req_data


def http_status_code_for_response(vins_response):
    for meta in vins_response.meta:
        if isinstance(meta, ErrorMeta) and meta.error_type in SENSITIVE_META_ERROR_TYPES:
            return SOMETHING_WRONG_HTTP_STATUS_CODE
        elif isinstance(meta, ErrorMeta) and meta.error_type == 'eou_expected':
            return EOU_EXPECTED_CODE
        elif meta.type == 'cancel_listening':
            return CANCEL_LISTENING_CODE
    return falcon.HTTP_200
