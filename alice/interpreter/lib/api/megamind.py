from __future__ import annotations

import json
import logging
from typing import Any, Optional

import attr
import requests

session = requests.Session()
logger = logging.getLogger(__name__)


@attr.s(auto_attribs=True)
class MMRequest:
    _url: str
    _raw_data: bytes
    headers: dict[str, str]
    timeout: int

    def __attrs_post_init__(self):
        self._data = json.loads(self._raw_data)

    @property
    def request_id(self) -> str:
        return self._data['header']['request_id']

    def get_utterance(self) -> Optional[str]:
        event = self._data['request']['event']

        if event['type'] == 'voice_input':
            if 'original_zero_asr_hypothesis_index' in event:
                hyp = event['original_zero_asr_hypothesis_index']
            else:
                hyp = 0
            if len(event['asr_result']) == 0:
                return None
            else:
                return ' '.join(word['value'] for word in event['asr_result'][hyp]['words'])

        elif event['type'] in ('text_input', 'suggested_input'):
            return event['text']
        else:
            return None

    def set_utterance(self, utterance: str) -> None:
        event = self._data['request']['event']

        if event['type'] == 'voice_input':
            event.pop('original_zero_asr_hypothesis_index', None)
            if len(event['asr_result']) >= 0:
                event['asr_result'].insert(0, {
                    'utterance': utterance,
                    'confidence': 1.0,
                    'words': [],
                    'normalized': utterance,
                })
        elif event['type'] in ('text_input', 'suggested_input'):
            event['text'] = utterance

    def get_user_gender(self) -> Optional[str]:
        event = self._data['request']['event']
        if 'biometry_classification' not in event:
            return None

        for classification in event['biometry_classification']['simple']:
            if classification['tag'] == 'gender':
                return classification['classname']
        return None

    def to_request(self) -> requests.Request:
        return requests.Request(
            'POST', self._url,
            data=json.dumps(self._data),
            headers=self.headers,
        )


@attr.s
class MMResponse:
    _data: dict[str, Any] = attr.ib(repr=False)
    headers: dict[str, str] = attr.ib(repr=False)
    request_id: str = attr.ib()
    code: int = attr.ib(repr=False)

    @classmethod
    def from_response(cls, response: requests.Response) -> MMResponse:
        data = response.json()
        return cls(
            data,
            request_id=data.get('header', {}).get('request_id', ''),
            headers=response.headers,
            code=response.status_code,
        )

    def get_voice(self) -> Optional[str]:
        if 'output_speech' in self._data.get('voice_response', {}):
            return self._data['voice_response']['output_speech'].get('text')

    def set_voice(self, voice) -> None:
        if 'output_speech' in self._data.get('voice_response', {}):
            out_speech = self._data['voice_response']['output_speech']
            out_speech['text'] = voice
            if 'type' not in out_speech:
                out_speech['type'] = 'simple'

    def get_cards_text(self) -> list[str]:
        res = []
        cards = self._data.get('response', {}).get('cards', [])
        for card in cards:
            res.append(card.get('text', ''))

        return res

    def set_cards_text(self, texts: list[str]) -> None:
        single_card = self._data.get('response', {}).get('card', None)
        cards = self._data.get('response', {}).get('cards', [])

        if len(texts) != len(cards):
            logger.error('You try to set %s texts, for a %s cards', len(texts), len(cards))
            return

        def set_text_to_card(card, text) -> None:
            if card.get('type') in ('simple_text', 'text_with_button', 'div_card'):
                card['text'] = text

        if single_card:
            set_text_to_card(single_card, texts[0])

        for card, text in zip(cards, texts):
            set_text_to_card(card, text)

    def get_suggests_text(self) -> list[str]:
        res = []
        suggests = self._data.get('response', {}).get('suggest', {}).get('items', [])
        for btn in suggests:
            res.append(btn.get('title', ''))

            # hack for yandex station
            directives = btn.get('directives', [])
            if directives and directives[0]['name'] == 'type':
                res.append(directives[0]['payload']['text'])
            else:
                res.append('')

        return res

    def set_suggests_text(self, texts: list[str]) -> None:
        suggests = self._data.get('response', {}).get('suggest', {}).get('items', [])
        if len(texts) != len(suggests * 2):
            logger.error('You try to set %s texts, for a %s suggests', len(texts), len(2 * suggests))
            return

        for i, btn in enumerate(suggests):
            btn['title'] = texts[i * 2]

            # hack for yandex station
            type_text = texts[i * 2 + 1]
            directives = btn.get('directives', [])
            if directives and directives[0]['name'] == 'type':
                directives[0]['payload']['text'] = type_text

    def to_json_string(self) -> str:
        return json.dumps(self._data, indent=2)


def ask_mm(mm_request: MMRequest) -> Optional[MMResponse]:
    try:
        req = session.prepare_request(mm_request.to_request())
        resp = session.send(req, timeout=mm_request.timeout)
    except Exception:
        logger.exception('Megamind request error %s', req.url)
        return None

    if not resp.ok:
        logger.error('Megamind response error %s %s %s', req.url, resp.status_code, resp.text)

    try:
        return MMResponse.from_response(resp)
    except Exception:
        logger.exception('Megamind response parse failed %s %s', req.url, resp.text)
        return None
