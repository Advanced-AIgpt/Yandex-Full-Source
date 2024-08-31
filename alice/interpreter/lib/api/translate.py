from __future__ import annotations

import logging
from typing import Optional, List, Dict

import attr
import requests

logger = logging.getLogger(__name__)
session = requests.Session()


@attr.s(auto_attribs=True)
class TranslateResult:
    request_text: str
    response_text: Optional[str]
    lang: str
    has_error: bool = False


def translate(text: str, lang: str, config: Dict[str, str], *, context_prompts: str = '') -> TranslateResult:
    return translate_many([text], lang, config, context_prompts=context_prompts)[0]


def translate_many(text: List[str], lang: str, config: Dict[str, str], *, context_prompts: str = '') -> List[TranslateResult]:
    if not text:
        return []

    src_lang, dst_lang = lang[0:2], lang[3:5]
    if src_lang == dst_lang:
        return [
            TranslateResult(
                request_text=txt,
                response_text=txt,
                lang=lang,
                has_error=False,
            ) for txt in text
        ]

    translate_api_url = config['SOURCE_TRANSLATE_URL']
    request_timeout = config['SOURCE_TRANSLATE_TIMEOUT']

    params = [
        ('lang', lang),
        ('srv', 'alice'),
    ] + [('text', t) for t in text]

    if int(config['TRANSLATE_ENABLE_PROMPTS']) and context_prompts:
        params.append(('context_prompts', context_prompts))

    request = requests.Request('GET', translate_api_url, params=params)
    has_error = False
    translate_result = []

    try:
        request = session.prepare_request(request)
        response = session.send(request, timeout=request_timeout)
    except Exception:
        logger.exception('Translate request error %s', request.url)
        has_error = True
        translate_result = [None] * len(text)
    else:
        if not response.ok:
            logger.error('Translate response error %s %s %s', request.url, response.status_code, response.text)
            has_error = True

        try:
            data = response.json()
            translate_result = data['text']
        except Exception:
            logger.exception('Invalid Translate response %s %s', request.url, response.text)
            has_error = True

    return [
        TranslateResult(
            request_text=request_text,
            response_text=None if has_error else response_text,
            lang=lang,
            has_error=has_error,
        ) for request_text, response_text in zip(text, translate_result)
    ]
