import alice.protos.endpoint.capabilities.route_manager.route_manager_pb2 as route_manager_pb2

from . import mixin


RouteManagerState = route_manager_pb2.TRouteManagerCapability.TState


class DirectivesMixin(
    mixin.SkillsDirectives,
):
    def route_manager_start_directive(self, data):
        self._device_state.RouteManagerState.Route = RouteManagerState.Moving

    def route_manager_stop_directive(self, data):
        self._device_state.RouteManagerState.Route = RouteManagerState.Stopping

    def route_manager_show_directive(self, data):
        self._device_state.RouteManagerState.Layout = RouteManagerState.Map

    def route_manager_continue_directive(self, data):
        self._device_state.RouteManagerState.Route = RouteManagerState.Moving

    def messenger_call(self, data):
        pass
