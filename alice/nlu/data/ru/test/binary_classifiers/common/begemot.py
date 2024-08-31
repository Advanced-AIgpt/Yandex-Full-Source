import requests


DEFAULT_RULES = [
    'AliceBegginsEmbedder',
    'AliceNormalizer',
    'AliceRequest',
]
DEFAULT_WIZEXTRA = []
DEFAULT_BEGEMOT_HOST = 'beggins.in.alice.yandex.net'


class BegemotClient:
    def __init__(self, host=None, rules=None, wizextra=None):
        self._host = host or DEFAULT_BEGEMOT_HOST
        self._rules = rules or DEFAULT_RULES
        self._wizextra = wizextra or DEFAULT_WIZEXTRA

    def query(self, text, debug_level=None):
        params = {
            'text': text,
            'wizclient': 'megamind',
            'tld': 'ru',
            'format': 'json',
            'lang': 'ru',
            'uil': 'ru',
            'dbgwzr': debug_level or 2,
            'wizextra': ';'.join(self._wizextra),
            'rwr': self._rules,
            'markup': '',
        }
        response = requests.get(f'http://{self._host}/wizard', params=params)
        print(response.request.url)
        response.raise_for_status()
        return response.json()
