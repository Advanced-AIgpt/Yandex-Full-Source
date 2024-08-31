# coding: utf-8

import json

import click

from library.python import resource
from alice.acceptance.modules.request_generator.scrapper.lib.api import request as api_request
from alice.acceptance.modules.request_generator.scrapper.lib.api import response as api_response
from alice.acceptance.modules.request_generator.scrapper.lib.api import run as api_run
from alice.uniproxy.mapper.test_bin.lib import code


TEST_DATA = json.loads(resource.find('data.json'))


def run(request_data_generator, session_builder):
    test_step = None
    test_name = None
    generated_request = None
    fulfilled_requests = None
    fulfilled_responses = None
    for request in request_data_generator():
        payload = request.payload
        if request.command == api_request.Command.START:
            fulfilled_requests = []
            fulfilled_responses = []
            test_step = 0
            test_name = payload.get(b'test_name', b'').decode('utf-8')
            session_builder.new_session()
        if test_name not in TEST_DATA:
            yield api_response.AliceNextRequestException('Unknown test_name: "{}"'.format(test_name))
            continue
        if test_step >= len(TEST_DATA[test_name]):
            yield api_response.AliceEndSession()
            continue

        if request.command == api_request.Command.CONTINUE:
            try:
                assert_func_name = '{}_{}_asserts'.format(test_name, test_step - 1)
                assert_func = getattr(code, assert_func_name)
                assert_func(generated_request, payload, fulfilled_requests, fulfilled_responses)
            except Exception as e:
                yield api_response.AliceEndSessionFailed(str(e))
                continue
            fulfilled_requests.append(generated_request)
            fulfilled_responses.append(payload)

        prev_response = None
        if request.command == api_request.Command.CONTINUE:
            prev_response = payload
        next_request_data = TEST_DATA[test_name][test_step]
        yield api_response.AliceNextRequest(session_builder.next_request(next_request_data, prev_response))
        test_step += 1


@click.command()
@click.option('--init-ctx', required=True)
def main(init_ctx):
    api_run.run(init_ctx, run)


if __name__ == '__main__':
    main()
