import alice.megamind.protos.scenarios.directives_pb2 as directives_pb2
import alice.protos.extensions.extensions_pb2 as extensions_pb2


class _DirectiveName(object):
    def __init__(self):
        directives = directives_pb2.TDirective.DESCRIPTOR.oneofs_by_name['Directive'].fields
        for d in directives:
            setattr(self, d.name, d.message_type.GetOptions().Extensions[extensions_pb2.SpeechKitName])
        setattr(self, 'MmStackEngineGetNextCallback', mm_stack_engine_get_next)
        setattr(self, 'UpdateSpaceActionsDirective', update_space_actions)

mm_stack_engine_get_next = '@@mm_stack_engine_get_next'
update_space_actions = 'update_space_actions'
names = _DirectiveName()
