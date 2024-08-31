# -*- coding: utf-8 -*-

import requests


valid_grammar = """form YANDEX.YES:
    root:
        да

"""

invalid_grammar = """form YANDEX.YES:
    root:
        [да
"""


def make_request_body(grammars, positive_tests=None, negative_tests=None):
    return {
        'grammars': grammars,
        'positive_tests': positive_tests or [],
        'negative_tests': negative_tests or [],
    }


def test_version(granet_server):
    url = granet_server.url + '/version'
    response = requests.get(url)
    assert response.status_code == 200


def test_compile_grammar(compile_grammar_url):
    request_json = make_request_body({'test_grammar': valid_grammar})
    response = requests.post(compile_grammar_url, json=request_json)
    assert response.status_code == 200
    response_json = response.json()
    result = response_json['result']
    assert 'grammar_base64' in result
    assert result['true_positives'] == []


def test_compiler_error(compile_grammar_url):
    request_json = make_request_body({'test_grammar': invalid_grammar})
    response = requests.post(compile_grammar_url, json=request_json)
    assert response.status_code == 400
    response_json = response.json()
    error = response_json['error']
    assert error['line_index'] == 3


def test_wont_compile_empty_grammar(compile_grammar_url):
    request_json = make_request_body({'test_grammar': ''})
    response = requests.post(compile_grammar_url, json=request_json)
    assert response.status_code == 400


def test_wont_compile_whitespace_grammar(compile_grammar_url):
    request_json = make_request_body({'test_grammar': '\t\n'})
    response = requests.post(compile_grammar_url, json=request_json)
    assert response.status_code == 400


def test_compile_grammar_with_number(compile_grammar_url):
    grammar = '''form test_number:
    root:
        тест $YANDEX.NUMBER
'''
    request_json = make_request_body({'test_grammar': grammar})
    response = requests.post(compile_grammar_url, json=request_json)
    assert response.status_code == 200
    response_json = response.json()
    result = response_json['result']
    assert 'grammar_base64' in result
