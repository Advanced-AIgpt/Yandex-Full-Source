import grpc
import logging
import threading
import time
from concurrent import futures

from alice.hollywood.library.python.testing.it_grpc.wrappers import GraphRequest

from apphost.lib.grpc.protos.service_pb2 import TPingRequest, TPingResponse
from apphost.lib.grpc.protos.service_pb2_grpc import TServantStub, TServantServicer, add_TServantServicer_to_server


logger = logging.getLogger(__name__)


class MockServicer(TServantServicer):
    def Invoke(self, request_iterator, context):
        reqs_protos = [r for r in request_iterator]
        assert len(reqs_protos) == 1  # XXX(sparkle): redo if not true
        req_proto = reqs_protos[0]

        req = GraphRequest(req_proto)
        resp = self.on_request(req)
        return resp.proto

    def Ping(self, request, context):
        return TPingResponse()

    def on_request(self, req):
        raise NotImplementedError()


class MockGrpcServer:
    def __init__(self, mock_servicer, port):
        self._mock_servicer = mock_servicer
        self._port = port

    def start(self):
        self._grpc_server = grpc.server(futures.ThreadPoolExecutor(max_workers=2))
        add_TServantServicer_to_server(self._mock_servicer, self._grpc_server)
        self._grpc_server.add_insecure_port(f'localhost:{self._port}')

        logger.debug('MockGrpcServer listening on port %s', self._port)
        self._grpc_server.start()
        self._grpc_server.wait_for_termination()
        logger.debug('MockGrpcServer finished.')

    def stop(self):
        logger.debug('Stopping MockGrpcServer...')
        self._grpc_server.stop(grace=True)
        logger.debug('Stopped MockGrpcServer')

    def ping(self):
        channel = grpc.insecure_channel(f'localhost:{self._port}')
        ping_res = None
        try:
            stub = TServantStub(channel)
            ping_res = stub.Ping(TPingRequest())
        except:
            pass
        return ping_res == TPingResponse()


class MockThread(threading.Thread):
    def __init__(self, mock_servicer, port):
        super(MockThread, self).__init__()
        self._server = MockGrpcServer(mock_servicer, port)
        self.daemon = True

    def run(self):
        logger.info('Starting thread...')
        try:
            self._server.start()
        except Exception as exc:
            logger.exception(exc)
        finally:
            logger.info('Thread finished.')

    def start_sync(self):
        # start the thread
        logger.info('Thread is about to start...')
        self.start()

        # ping until success
        while True:
            logger.info('Pinging MockGrpcServer...')
            if self._server.ping():
                break
            time.sleep(0.1)
        logger.info('MockGrpcServer is ready.')

    def stop_sync(self):
        logger.info('MockGrpcServer is about to stop...')
        self._server.stop()
        self.join(300)
        logger.info('MockGrpcServer stopped.')
