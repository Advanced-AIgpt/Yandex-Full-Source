import grpc

from alice.hollywood.library.python.testing.it_grpc.mock_server import MockThread
from alice.hollywood.library.python.testing.it_grpc.wrappers import GraphResponse
from apphost.lib.grpc.protos.service_pb2_grpc import TServantStub


class Graph:
    def __init__(self, servers, port_manager, mock_dict, graph_name):
        self._servers = servers
        self._port_manager = port_manager
        self._mock_dict = mock_dict or {}
        self._graph_name = graph_name

        self._srcrwr_ports = {
            'HOLLYWOOD_ALL': self._servers.hollywood.grpc_port,
        }

    def start_mock_servers(self):
        self._mock_threads = []
        for alias, cls in self._mock_dict.items():
            # get port
            port = self._port_manager.get_port()
            self._srcrwr_ports[alias] = port

            # start thread
            t = MockThread(mock_servicer=cls(), port=port)
            t.start_sync()
            self._mock_threads.append(t)

    def stop_mock_servers(self):
        for t in self._mock_threads:
            t.stop_sync()

    def __call__(self, request):
        proto = request.proto
        proto.Path = self._build_path()

        # add test flag
        test_flag = proto.MetaFlags.add()
        test_flag.SourceName = 'INIT'
        test_flag.FlagName = 'it_grpc_test'

        channel = grpc.insecure_channel(f'localhost:{self._servers.apphost.grpc_port}')
        stub = TServantStub(channel)
        response = stub.Invoke((r for r in [proto]))

        return GraphResponse(request, response)

    def _build_path(self):
        path = ''
        for alias, port in self._srcrwr_ports.items():
            if path:
                path += '/'
            path += f'_srcrwr={alias}%3Agrpc%3A%2F%2Flocalhost%3A{port}'
        path += f'/{self._graph_name}'
        return path
