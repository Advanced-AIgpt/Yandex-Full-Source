# coding: utf-8

import json
import sys

import cyson

from alice.acceptance.modules.request_generator.scrapper.lib import io
from alice.acceptance.modules.request_generator.scrapper.lib import session

from alice.acceptance.modules.request_generator.scrapper.lib.api import request
from alice.acceptance.modules.request_generator.scrapper.lib.api import response


def set_error_response(message, output=sys.stdout.buffer):
    if not isinstance(message, str):
        message = 'Internal error, bad error message: "{}"'.format(repr(message))
    response_obj = response.AliceNextRequestException(message)
    io.write(cyson.dumps(response_obj.to_dict()), output)


def set_response(response: response.Response, output=sys.stdout.buffer):
    response_dict = response.to_dict()
    try:
        io.write(cyson.dumps(response_dict), output)
    except Exception as e:
        message = 'Yson serialize exception: "{}", invalid response: "{}"'.format(e, response_dict)
        set_error_response(message)
        return


def get_request_data() -> request.Request:
    for raw_request in io.read():
        try:
            parsed_yson = cyson.loads(raw_request.strip())
            request_obj = request.Request(
                command=request.Command(parsed_yson[b'command']),
                payload=parsed_yson[b'payload'],
            )
        except KeyError as e:
            message = 'Bad request format, required field missed "{}"'.format(e)
            set_error_response(message)
            continue
        except Exception as e:
            message = 'Yson parse exception: "{}", invalid request: "{}"'.format(e, repr(raw_request))
            set_error_response(message)
            continue
        yield request_obj


def run(init_ctx, runner):
    try:
        init_ctx = json.loads(init_ctx)
    except Exception as e:
        message = 'Init context json parse error: "{}", context "{}"'.format(e, repr(init_ctx))
        set_error_response(message)
        raise

    try:
        session_builder = session.AliceSessionBuilder(init_ctx)
        for response_obj in runner(get_request_data, session_builder):
            set_response(response_obj)
    except Exception as e:
        set_error_response('Unexpected exception: {}'.format(repr(e)))
        raise
