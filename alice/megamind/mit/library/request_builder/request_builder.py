from base64 import b64encode
import copy
from typing import Optional, Dict, Any

from alice.acceptance.modules.request_generator.lib import (
    app_presets,
    vins as mm_request_generator,
)
from alice.library.python.testing.megamind_request.input_dialog import AbstractInput
from alice.megamind.library.python.testing.session_builder import SessionBuilder
import alice.megamind.mit.library.common.names.directives as directive_names
from alice.megamind.mit.library.response import ResponseWrapper
from alice.megamind.mit.library.util import (
    Json,
    get_dummy_proto,
    get_value_by_path,
)


DEFAULT_LANG: str = 'ru-RU'
TEST_UUID: str = 'deadbeef-dead-beef-1234-deadbeef1234'
FAKE_SCENARIO_NAME: str = '_mit_fake_scenario_'

PERMANENT_EXPERIMENTS = {
    'mm_enable_protocol_scenario=TestScenario': '1'
}


class MegamindRequestBuilder:

    def __init__(self, input: Optional[AbstractInput] = None):
        self._input_obj: Optional[AbstractInput] = input
        self._request_id: Optional[str] = None
        self._lang: str = DEFAULT_LANG
        self._app_preset: app_presets.AppPreset = app_presets.DEFAULT_APP
        self._session_builder: Optional[SessionBuilder] = None
        self._raw_session: Optional[str] = None
        self._is_apply: bool = False
        self._experiments = PERMANENT_EXPERIMENTS.copy()
        self._memento: Optional[str] = None

    def add_experiments(self, experiments: Dict[str, str]):
        for key, value in experiments.items():
            self._experiments[key] = value
        return self

    def set_input(self, input_obj: AbstractInput):
        self._input_obj = input_obj
        return self

    def set_preset(self, app_preset: app_presets.AppPreset):
        self._app_preset = app_preset
        return self

    def set_request_id(self, request_id: str):
        self._request_id = request_id
        return self

    def set_lang(self, lang: str):
        self._lang = lang
        return self

    def set_stack_engine(self, stack_engine_proto):
        self._force_session_builder().set_stack_engine(stack_engine_proto)
        return self

    def set_scenario_state(self, scenario_name: str, state_proto: Any):
        self._force_session_builder().set_scenario_state(scenario_name, state_proto)
        return self

    def set_memento(self, memento_proto: Any):
        self._memento = b64encode(memento_proto.SerializeToString()).decode()
        return self

    @property
    def is_apply(self) -> bool:
        return self._is_apply

    def build(self) -> Json:
        assert self._input_obj, 'Megamind request should have input object to make event'
        assert self._request_id, 'Request id not set'

        event = self._input_obj.make_event()
        sk_req = mm_request_generator.make_vins_request(
            self._request_id,
            text=None,
            event=event,
            lang=self._lang,
            app=self._app_preset.application,
            oauth_token=self._app_preset.auth_token,
            uuid=TEST_UUID,
            session=self._get_or_build_session(),
            experiments=self._experiments,
            memento=self._memento,
        )
        return sk_req

    def apply(self, response: ResponseWrapper):
        directive = response.get_directive(directive_names.DEFER_APPLY)
        assert directive, 'Trying to make apply without defer_apply directive from previous response'
        self_copy = copy.deepcopy(self)
        raw_session = get_value_by_path(directive, 'payload.session')
        assert isinstance(raw_session, str), f'Session should be str, got {type(raw_session)}'
        self_copy._set_raw_session(raw_session)
        self_copy._set_is_apply(True)
        return self_copy

    def _get_or_build_session(self) -> Optional[str]:
        if self._raw_session:
            return self._raw_session
        if self._session_builder:
            return self._session_builder.build()
        return None

    def _set_raw_session(self, raw_session: str):
        self._raw_session = raw_session

    def _set_is_apply(self, is_apply: bool):
        self._is_apply = is_apply

    def _force_session_builder(self):
        if not self._session_builder:
            self._session_builder = SessionBuilder()
            self._session_builder.set_scenario_state(FAKE_SCENARIO_NAME, get_dummy_proto())
        return self._session_builder
