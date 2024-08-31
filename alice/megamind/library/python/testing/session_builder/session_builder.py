import base64
import json
import logging
from typing import Any
import zlib

from alice.megamind.library.session.protos.session_pb2 import TSessionProto


logger = logging.getLogger(__name__)

FIELD_MEGAMIND: str = '__megamind__'


class SessionBuilder:

    def __init__(self):
        self._proto: Any = TSessionProto()

    def set_stack_engine(self, stack_engine_proto):
        self._proto.StackEngineCore.CopyFrom(stack_engine_proto)

    def build(self):
        logger.info(f'Session proto: {self._proto}')
        sessions = dict()
        sessions[FIELD_MEGAMIND] = base64.b64encode(self._proto.SerializeToString()).decode('utf-8')
        serialized = zlib.compress(json.dumps(sessions).encode('utf-8'))
        encoded = base64.b64encode(serialized)
        logger.info(f'Encoded session: {encoded}')
        return encoded

    def set_scenario_state(self, scenario_name: str, state_proto: Any):
        self._proto.PreviousScenarioName = scenario_name
        scenario_session = TSessionProto.TScenarioSession()  # type: ignore
        scenario_session.State.State.Pack(state_proto)
        self._proto.ScenarioSessions[scenario_name].CopyFrom(scenario_session)
