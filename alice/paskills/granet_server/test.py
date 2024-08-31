# -*- coding: utf-8 -*-
#!/usr/bin/env python

from __future__ import print_function

import json
import sys

import requests
from pprint import pprint

def main():
    with open('grammar.grnt') as f:
        grammar = f.read()
    with open('grammar_2.grnt') as f:
        grammar_2 = f.read()
    data = {
        'grammars': {
            'grammar.grnt': grammar,
            'grammar_2.grnt': grammar_2,
        },
        'positive_tests': [
            'мама мыла раму',
            'мама мыла раму 2',
        ],
        'negative_tests': [
            'мама не мыла раму',
        ],
    }
    response = requests.post('http://localhost:13001/granet/compile', json=data)
    sys.stderr.write(str(response.status_code) + '\n')
    try:
        print(json.dumps(json.loads(response.text), indent=4, ensure_ascii=False))
    except:
        print(response.text)
        raise


if __name__ == '__main__':
    main()
