import requests
import json
import time
from urllib.parse import urlencode


def make_uri() -> str:
    host = 'ci-begemot-megamind-0-0.yappy.beta.yandex.ru'
    url = f'http://{host}/wizard'

    rules = ['AliceBegginsEmbedder', 'AliceBinaryIntentClassifier', 'Text']
    wizextra = ['bg_enumerate_dev_classifiers']

    params = {
        'uil': 'ru',
        'text': 'text',
        'wizclient': 'megamind',
        'format': 'json',
        'tld': 'ru',
        'rwr': rules,
        'wizextra': ';'.join(wizextra),
    }

    uri = url + '?' + urlencode(params, True)

    return uri


def get_begemot_classifier_name(model_meta_info: dict) -> str:
    model_name = model_meta_info['model_name']
    classifier_name = ''.join([part.title() for part in model_name.split('_')])
    embedder = model_meta_info['embedder']
    process_id = model_meta_info['process_id']
    return f'Alice{embedder}{classifier_name}__{process_id}'


def wait_fast_data(model_meta_info: dict):
    begemot_classifier_name = get_begemot_classifier_name(model_meta_info)

    SLEEP_TIME = 60  # 60 seconds

    while True:
        response = requests.get(url=make_uri())
        if response.status_code != 200:
            raise Exception(f'The request to begemot was not successful: get {response.status_code} status code')

        response = json.loads(response.text)

        try:
            dev_classifiers = response['rules']['AliceBinaryIntentClassifier']['DevClassifiers']
        except KeyError:
            raise Exception('The begemot response does not contain `DevClassifiers`')

        for classifier in dev_classifiers:
            if classifier == begemot_classifier_name:
                return
        time.sleep(SLEEP_TIME)
