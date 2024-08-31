# -*-coding: utf8 -*-
#!/usr/bin/env python

import argparse
import json
import sys
import requests
import nirvana.job_context as nv

reload(sys)
sys.setdefaultencoding('utf-8')

SPELLER = "https://speller.yandex.net/services/spellservice.json/checkText"

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


def check_spelling(text):
    params = {"text": text}
    resp = requests.post(SPELLER, params=params, verify=False)
    return len(resp.json())

def get_literacy(assignments):
    res = {}

    for item in assignments:
        worker_id = item['workerId']
        text = item['outputValues'].get('annotation') or item['outputValues'].get('query')
        misspells = check_spelling(text)
        if worker_id in res:
            res[worker_id]['misspells'] += misspells
            res[worker_id]['texts'] += 1
        else:
            res[worker_id] = {'misspells': misspells, 'texts': 1}

    return res


def main():
    with open(inputs.get("input1"), "r") as f:
        assignments = json.load(f)

    result = get_literacy(assignments)

    with open(outputs.get('output1'), 'w') as f:
        json.dump(result, f, indent=4, ensure_ascii=False)


if __name__ == '__main__':
    main()
