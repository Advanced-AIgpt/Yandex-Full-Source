import alice.protos.extensions.extensions_pb2 as extensions_pb2
from cached_property import cached_property

from .response import Response


class HollywoodResponse(Response):
    def __init__(self, message, scenario_name):
        assert message.scenario_stages() == {'run'}
        self._o = message.run_response.ResponseBody
        self._scenario_name = scenario_name

    @property
    def text(self):
        return self.card.Text if self.card else None

    def has_voice_response(self):
        return True

    @property
    def voice_response(self):
        return None

    @property
    def output_speech_text(self):
        return self._o.Layout.OutputSpeech

    @property
    def scenario(self):
        return self._scenario_name

    @property
    def product_scenario(self):
        return self._o.AnalyticsInfo.ProductScenarioName

    @property
    def intent(self):
        return self._o.AnalyticsInfo.Intent

    @cached_property
    def directives(self):
        return [DirectiveWrapper(d) for d in self._o.Layout.Directives]

    @property
    def cards(self):
        return self._o.Layout.Cards

    @property
    def raw(self):
        return self._o


class DirectiveWrapper(object):

    def __init__(self, directive):
        self._o = getattr(directive, directive.WhichOneof('Directive'))

    def __getattr__(self, key):
        return self._o.__getattribute__(key)

    @property
    def name(self):
        return self.DESCRIPTOR.GetOptions().Extensions[extensions_pb2.SpeechKitName]
