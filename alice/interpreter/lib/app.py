from __future__ import annotations

import logging
import re
from urllib.parse import urlunsplit, urlsplit
from http import HTTPStatus
from typing import Any

from flask import Flask, Request, Response, request as current_request, jsonify, abort

from alice.interpreter.lib.api.translate import translate, translate_many
from alice.interpreter.lib.api.megamind import MMRequest, MMResponse, ask_mm

app = Flask(__name__)
logger = logging.getLogger(__name__)


@app.route('/ping')
def ping():
    return 'Ok'


@app.route('/', methods=['GET', 'POST'])
@app.route('/<path:dummy>', methods=['GET', 'POST'])
def proxy(dummy=''):
    if current_request.method == 'GET':
        return jsonify(app.raw_config)

    if current_request.content_length > 1024 * 1024:
        abort(HTTPStatus.BAD_REQUEST)

    config = build_config_for_request(current_request, app.config)
    logger.debug("Config: %s", config)

    # pipeline: Req -> get mm req -> patch -> get mm resp -> patch -> Resp
    mm_request = prepare_mm_request(current_request, config)
    logger.info('Got megamind request %s %s', mm_request.request_id, current_request.url)
    patch_mm_request(mm_request, config)

    mm_response = ask_mm(mm_request)

    if mm_response is None:
        abort(HTTPStatus.INTERNAL_SERVER_ERROR)

    patch_mm_response(mm_request, mm_response, config)

    response = Response(
        mm_response.to_json_string(),
        status=mm_response.code,
    )

    for key, value in mm_response.headers.items():
        if key in ('Connection', 'Content-Encoding', 'Transfer-Encoding'):
            continue

        response.headers[key] = value

    return response


def build_config_for_request(request: Request, default_config: dict[str, Any]) -> dict[str, Any]:
    """
    Overrides default config parameters with URL's query parameters
    """
    config = default_config.copy()
    for key in config.keys():
        if key in request.args:
            config[key] = request.args.get(key)

    # srcrwr=MEGAMIND:host:port:timeout&srcrwr=TEST:host:port
    rwrs = dict(rwr.split(':', 1) for rwr in request.args.getlist('srcrwr'))
    mm_url = config['SOURCE_MEGAMIND_URL']
    netloc = urlsplit(mm_url).netloc
    timeout = config['SOURCE_MEGAMIND_TIMEOUT']

    if 'MEGAMIND' in rwrs:
        proxy_settings = rwrs['MEGAMIND'].split(':')

        if len(proxy_settings) == 3:
            host, port, timeout = proxy_settings
            timeout = int(timeout)
        elif len(proxy_settings) == 2:
            host, port = proxy_settings
        elif len(proxy_settings) == 1:
            host = proxy_settings[0]
            port = 80
        else:
            abort(Response(f'Bad srcrwr {request.query_string}', status=HTTPStatus.BAD_REQUEST))
        netloc = f'{host}:{port}'

    mm_url = urlunsplit((
        'http',
        netloc,
        request.path,
        '', '',
    ))

    config['SOURCE_MEGAMIND_URL'] = mm_url
    config['SOURCE_MEGAMIND_TIMEOUT'] = timeout
    return config


def prepare_mm_request(request: Request, request_config: dict[str, Any]) -> MMRequest:
    # Do not forward header for target virtual host
    headers = dict(request.headers)
    if 'Host' in headers:
        del headers['Host']

    return MMRequest(
        request_config['SOURCE_MEGAMIND_URL'],
        request.get_data(),
        headers,
        request_config['SOURCE_MEGAMIND_TIMEOUT'],
    )


def patch_mm_request(request: MMRequest, request_config: dict[str, Any]) -> None:
    utt = request.get_utterance()
    if utt is None:
        logger.debug('Utterance is empty, skip. Reqid: %s', request.request_id)
        return

    translation_context_prompts = request_config['TRANSLATE_REQUEST_PROMPTS']
    gender = request.get_user_gender()
    if gender is not None and 'first_person' not in translation_context_prompts:
        translation_context_prompts = ','.join(filter(None, [
            translation_context_prompts,
            'first_person:' + request_config['GENDER_TO_PROMPT_MAPPING'][gender],
        ]))

    trans_utt = translate(utt, request_config['TRANSLATE_REQUEST_LANG'], request_config, context_prompts=translation_context_prompts)
    if trans_utt.has_error:
        logger.error('Utterance was not changed due to translate api error.')
        return

    logger.info('Translate utterance from "%s" to "%s"', utt, trans_utt.response_text)
    request.set_utterance(trans_utt.response_text)


def patch_mm_response(request: MMRequest, response: MMResponse, request_config: dict[str, Any]) -> None:
    ru_voice = response.get_voice() or ''
    ru_voice = re.sub('<.*?>', '', ru_voice)

    texts = response.get_cards_text()
    suggests = response.get_suggests_text()

    to_translate = [ru_voice] + texts + suggests
    if to_translate == ['']:  # only empty voice
        logger.info('Nothing to translate, probably defer_apply')
        return

    translation_context_prompts = request_config['TRANSLATE_RESPONSE_PROMPTS']
    gender = request.get_user_gender()
    if gender is not None and 'second_person' not in translation_context_prompts:
        translation_context_prompts = ','.join(filter(None, [
            translation_context_prompts,
            'second_person:' + request_config['GENDER_TO_PROMPT_MAPPING'][gender],
        ]))

    trans_res = translate_many(to_translate, request_config['TRANSLATE_RESPONSE_LANG'], request_config, context_prompts=translation_context_prompts)

    voice_trans = trans_res[0]
    cards_trans = trans_res[1:len(texts) + 1]
    suggests_trans = trans_res[len(texts) + 1:]

    if voice_trans.has_error:
        logger.error('Response was not changed due to translate api error.')
        return

    en_voice = '{} {}'.format(
        request_config['VOICE_PREFIX'],
        voice_trans.response_text
    )
    logger.info('Translate output speech from "%s" to "%s"', ru_voice, en_voice)

    response.set_voice(en_voice)
    response.set_cards_text([res.response_text for res in cards_trans])
    response.set_suggests_text([res.response_text for res in suggests_trans])
