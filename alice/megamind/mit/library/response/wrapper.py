from typing import Optional, List

from alice.megamind.mit.library.util import (
    Json,
    get_value_by_path,
)


class ResponseWrapper:

    def __init__(self, response_json: Json):
        self._response_json = response_json

    @property
    def card(self) -> Optional[Json]:
        return get_value_by_path(self._response_json, 'response.card')

    @property
    def output_speech(self):
        return get_value_by_path(self._response_json, 'voice_response.output_speech.text')

    @property
    def error(self) -> Optional[Json]:
        return get_value_by_path(self._response_json, 'error')

    @property
    def winner_scenario(self):
        return get_value_by_path(self._response_json, 'megamind_analytics_info.winner_scenario.name')

    def _get_directives(self) -> List[Json]:
        directives = get_value_by_path(self._response_json, 'response.directives')
        assert isinstance(directives, list)
        return directives

    def get_directive(self, name: str) -> Optional[Json]:
        for directive in self._get_directives():
            assert isinstance(directive, dict)
            if directive.get('name') == name:
                return directive
        return None
