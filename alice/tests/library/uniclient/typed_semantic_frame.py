import alice.megamind.protos.common.frame_pb2 as frame_pb2

from .proto_wrapper import ProtoWrapper


class _TypedSemanticFrame(ProtoWrapper):
    proto_cls = frame_pb2.TTypedSemanticFrame

    def __init__(self, name, value):
        super().__init__({name: value})


class _Frame(object):
    def __init__(self):
        frames = frame_pb2.TTypedSemanticFrame.DESCRIPTOR.oneofs_by_name['Type'].fields
        for f in frames:
            setattr(
                self,
                f.name,
                (lambda name: lambda **kwargs: _TypedSemanticFrame(name, kwargs))(f.json_name),
            )


frame = _Frame()
