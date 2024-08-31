import alice.megamind.library.config.scenario_protos.rpc_handler_config_pb2 as rpc_handler_proto
import alice.library.python.utils as utils
from alice.tests.library.uniclient import ProtoWrapper
from cached_property import cached_property


class RpcHandler(ProtoWrapper):
    proto_cls = rpc_handler_proto.TRpcHandlerConfigProto

    @cached_property
    def snake_case_name(self):
        return utils.to_snake_case(self.HandlerName)
