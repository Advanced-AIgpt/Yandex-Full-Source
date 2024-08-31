import alice.megamind.library.config.scenario_protos.combinator_config_pb2 as combinator_proto
import alice.library.python.utils as utils
from alice.tests.library.uniclient import ProtoWrapper
from cached_property import cached_property


class Combinator(ProtoWrapper):
    proto_cls = combinator_proto.TCombinatorConfigProto

    @cached_property
    def snake_case_name(self):
        return utils.to_snake_case(self.Name)

    @cached_property
    def graph_prefix(self):
        return f'combinator_{self.snake_case_name}'

    @cached_property
    def input_deps(self):
        return set([_.NodeName for _ in self.Dependences])

    @cached_property
    def combinators_setup(self):
        return [_ for _ in self.Dependences if _.NodeName == 'COMBINATORS_SETUP']

    @cached_property
    def dependences(self):
        return [_ for _ in self.Dependences if _.NodeName != 'COMBINATORS_SETUP']
