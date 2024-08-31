import hashlib
import logging
import requests

logger = logging.getLogger(__name__)


class SpeechKit:
    def __init__(self, megamind, joker):
        self.megamind = megamind
        self.joker = joker

    def request(self, data):
        headers = {
            'content-type': 'application/json; charset=utf-8',
        }
        self.joker.insert_headers(headers, hashlib.sha256(data.encode('utf-8')).hexdigest())
        resp = requests.post(self.megamind.make_url('speechkit/app/pa/'), data=data.encode('utf-8'), headers=headers)
        return resp
