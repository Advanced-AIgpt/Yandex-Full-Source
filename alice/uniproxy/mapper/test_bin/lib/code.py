import json


def hello_and_repeat_0_asserts(request, response, prev_requests, prev_responses):
    pass


def hello_and_repeat_1_asserts(request, response, prev_requests, prev_responses):
    assert prev_responses, 'Empty prev responses data'
    request_text = request['Text']
    assert request_text == 'повтори', 'Non repeat test case'
    try:
        response_text = json.loads(response[b'VinsResponse'])['directive']['payload']['response']['card']['text']
    except Exception as e:
        assert False, 'Exception getting text from response %s' % e
    try:
        prev_response_text = json.loads(
            prev_responses[-1][b'VinsResponse']
        )['directive']['payload']['response']['card']['text']
    except Exception as e:
        assert False, 'Exception getting text from prev response %s' % e
    assert response_text == prev_response_text, 'Unequal response text [%s] and prev response text [%s]' % (
        response_text, prev_response_text
    )
