from infra.yp_service_discovery.api import api_pb2
from infra.yp_service_discovery.python.resolver.resolver import Resolver
import argparse
import json
import socket


if __name__ == '__main__':
    # parse arguments
    parser = argparse.ArgumentParser(description='I will give you the endpoint!')
    parser.add_argument('--cluster-name', dest='cluster_name', required=True)
    parser.add_argument('--endpoint-set-id', dest='endpoint_set_id', required=True)
    args = parser.parse_args()

    # send request
    resolver = Resolver(client_name='test:{}'.format(socket.gethostname()), timeout=5)
    request = api_pb2.TReqResolveEndpoints()
    request.cluster_name = args.cluster_name
    request.endpoint_set_id = args.endpoint_set_id

    result = resolver.resolve_endpoints(request)

    # print an endpoint if found
    endpoints = result.endpoint_set.endpoints

    if len(endpoints) > 0:
        endpoint = endpoints[0]
        result = {
            'status': 'OK',
            'host': endpoint.fqdn,
            'port': endpoint.port
        }
    else:
        result = {
            'status': 'ERROR'
        }

    print(json.dumps(result))
