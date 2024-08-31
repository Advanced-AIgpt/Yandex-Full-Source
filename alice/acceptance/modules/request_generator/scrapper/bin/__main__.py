# coding: utf-8

import click

from alice.acceptance.modules.request_generator.scrapper.lib.api import request as api_request
from alice.acceptance.modules.request_generator.scrapper.lib.api import response as api_response
from alice.acceptance.modules.request_generator.scrapper.lib.api import run as api_run


def run(request_data_generator, session_builder):
    step = None
    session_requests = None
    for request in request_data_generator():
        try:
            payload = request.payload
            if request.command == api_request.Command.START:
                step = 0
                session_requests = payload[b'session_requests']
                session_builder.new_session()
            if request.command != api_request.Command.START and session_requests is None:
                yield api_response.AliceNextRequestException(
                    'Invalid command "{}", "start" command missed or session_request is empty'.format(
                        request.command
                    ))
                continue
            if step >= len(session_requests):
                yield api_response.AliceEndSessionOk()
                continue

            prev_response = None
            if request.command == api_request.Command.CONTINUE:
                prev_response = payload
            next_request_data = session_requests[step]
            next_request_obj = session_builder.next_request(next_request_data, prev_response)
            yield api_response.AliceNextRequest(next_request_obj)
            step += 1
        except Exception as e:
            yield api_response.AliceNextRequestException(
                'Unexpected generating "next_request" exception: "{}"'.format(repr(e))
            )


@click.command()
@click.option('--init-ctx', required=True)
def main(init_ctx):
    api_run.run(init_ctx, run)


if __name__ == '__main__':
    main()
