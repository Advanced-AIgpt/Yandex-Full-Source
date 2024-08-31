import requests
import json

from .cfg import WIZARD_CONFIG


def __extract(wizard_response):
    rules = wizard_response.get('rules', {})
    CommercialMx = rules.get('CommercialMx', {})
    commercialMx = CommercialMx.get('commercialMx', 0)
    return float(commercialMx)


def __curl_wizard(text, proxy, port, wizclient, lr, wizard_output=None, verbose=0, **args):
    params = {
        'format': 'json',
        'text': text,
        'wizclient': wizclient,
        'lr': lr
    }

    url = 'http://{}:{}/wizard'.format(proxy, port)
    req = requests.get(
        url,
        params=params,
    )
    if verbose > 1:
        print(req.url)
    response = req.json()
    if wizard_output:
        with open(wizard_output, 'a') as f:
            f.write(json.dumps(response, ensure_ascii=False) + '\n')

    return __extract(response)


def ask_wizard(text, **args):
    response = __curl_wizard(text, **WIZARD_CONFIG, **args)
    is_above_threshold = 'CROP' if response >= WIZARD_CONFIG['threshold'] else 'OK'
    return is_above_threshold


if __name__ == "__main__":
    query = 'навук гороскоп'
    ask_wizard(query, verbose=2)
