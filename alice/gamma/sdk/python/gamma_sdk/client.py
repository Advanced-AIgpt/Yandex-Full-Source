# coding:utf-8
import argparse

import grpc
import json
import logging
import logging.config
from concurrent import futures

import alice.gamma.sdk.api.api_pb2
import alice.gamma.sdk.api.api_pb2_grpc
import alice.gamma.sdk.api.commands_pb2_grpc

from gamma_sdk.sdk import sdk
from gamma_sdk.log import create_config, get_log_adapter


def start_skill(skill):
    parser = argparse.ArgumentParser()
    parser.add_argument('--sdk-addr', default='localhost:8002')
    parser.add_argument('--port', default='8001')
    parser.add_argument('--max-workers', '-n', default=10)
    parser.add_argument('--sdk-timeout', default=0.25)
    parser.add_argument('--log-level', choices=list(logging._nameToLevel.keys()), default=logging.DEBUG)
    parser.add_argument('--log-json', action="store_true", default=False)
    parser.add_argument('--grace', dest='gracefulness', default=100)
    args = parser.parse_args()

    logging.config.dictConfig(create_config(args.log_level, args.log_json))
    _start_skill(skill, args)


class _SdkClient:
    def __init__(self, addr):
        self.addr = addr
        self._channel = None

    def connect(self):
        self._channel = grpc.insecure_channel(self.addr)
        return alice.gamma.sdk.api.commands_pb2_grpc.SdkStub(self._channel)

    def close(self):
        if self._channel:
            self._channel.close()

    def __del__(self):
        self.close()


class _SkillHandler(alice.gamma.sdk.api.api_pb2_grpc.SkillServicer):
    def __init__(self, skill, sdk_client, logger, sdk_timeout=0.25):
        self.sdk_client = sdk_client
        self.skill = skill
        self.logger = logger
        self.sdk_timeout = sdk_timeout
        super(_SkillHandler, self).__init__()

    def Handle(self, request, ctx):
        try:
            request_body = sdk.Request.from_proto(request.request)
            meta = sdk.Meta.from_proto(request.meta)

            stub = self.sdk_client.connect()

            uuid = request.session.userId
            skill_id = request.session.skillId
            session_id = request.session.sessionId
            logger = get_log_adapter(self.logger, uuid=uuid, skill_id=skill_id, session_id=session_id)

            try:
                state_data = from_proto(request.state)
            except ValueError as e:
                logger.error('Failed to restore state: %s', e)
                state_data = {}
            state = self.skill.state_cls.from_dict(state_data)

            logger.debug('Received request: %s %s %s', request_body, meta, state)

            context = sdk.SkillContext(logger, ctx, stub, request.session, state, self.sdk_timeout)
            response = self.skill.handle(
                logger,
                context,
                request_body,
                meta
            )

            return alice.gamma.sdk.api.api_pb2.SkillResponse(
                response=response.to_proto(),
                session=request.session,
                state=to_proto(state.to_dict())
            )
        finally:
            self.sdk_client.close()


def _start_skill(skill, args):
    client = _SdkClient(args.sdk_addr)
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=args.max_workers))

    logger = logging.getLogger('skill_logger')
    logger.info('Starting')
    alice.gamma.sdk.api.api_pb2_grpc.add_SkillServicer_to_server(
        _SkillHandler(skill=skill, sdk_client=client, logger=logger, sdk_timeout=args.sdk_timeout), server
    )
    server.add_insecure_port('[::]:%s' % args.port)
    server.start()
    try:
        while True:
            pass
    except KeyboardInterrupt:
        logger.info('Stopping')
        server.stop(grace=args.gracefulness)


def from_proto(proto):
    if proto.storage:
        storage = json.loads(proto.storage)
        if not isinstance(storage, dict):
            raise ValueError('Could not deserialize state')
    else:
        storage = {}

    return storage


def to_proto(state):
    return alice.gamma.sdk.api.api_pb2.State(
        storage=json.dumps(state, ensure_ascii=False).encode('utf-8')
    )
