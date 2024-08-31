import alice.megamind.protos.common.atm_pb2 as atm_pb2

from .proto_wrapper import ProtoWrapper


EOrigin = atm_pb2.TAnalyticsTrackingModule.EOrigin


class _Analytics(ProtoWrapper):
    proto_cls = atm_pb2.TAnalyticsTrackingModule


class Web(_Analytics):
    def __init__(self, **kwargs):
        super().__init__(origin=EOrigin.Web, **kwargs)


class Scenario(_Analytics):
    def __init__(self, **kwargs):
        super().__init__(origin=EOrigin.Scenario, **kwargs)


class SmartSpeaker(_Analytics):
    def __init__(self, **kwargs):
        super().__init__(origin=EOrigin.SmartSpeaker, **kwargs)


class SearchApp(_Analytics):
    def __init__(self, **kwargs):
        super().__init__(origin=EOrigin.SearchApp, **kwargs)


class RemoteControl(_Analytics):
    def __init__(self, **kwargs):
        super().__init__(origin=EOrigin.RemoteControl, **kwargs)


class Proactivity(_Analytics):
    def __init__(self, **kwargs):
        super().__init__(origin=EOrigin.Proactivity, **kwargs)
