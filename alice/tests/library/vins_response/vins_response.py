import alice.tests.library.scenario as scenario
from cached_property import cached_property

from .analytics_info import MegamindAnalyticsInfo
from .div_card import div_card_types, DivCard
from .response import Response


class VinsResponse(Response):
    def __init__(self, message):
        header = message.directive.header
        assert header.name == self.__class__.__name__
        self._o = message.directive.payload

    @property
    def text(self):
        return self.card.text if self.card else None

    @cached_property
    def buttons(self):
        return self.card.get('buttons', []) if self.card else None

    def button(self, title):
        for b in self.buttons:
            if b.title == title:
                return b

    def has_voice_response(self):
        return self.voice_response.get('output_speech') is not None

    @property
    def voice_response(self):
        return self._o.voice_response

    @property
    def output_speech_text(self):
        if not self.has_voice_response():
            return None
        return self.voice_response.output_speech.text

    @property
    def scenario(self):
        winner_scenario = self._o.megamind_analytics_info.get('winner_scenario')
        return winner_scenario.name if winner_scenario else None

    @property
    def combinator_product_name(self):
        combinator_analytics = self._o.megamind_analytics_info.get('combinators_analytics_info')
        return combinator_analytics.combinator_product_name if combinator_analytics else None

    @property
    def product_scenario(self):
        return self.scenario_analytics_info.product_scenario

    @property
    def intent(self):
        return self.scenario_analytics_info.intent

    @property
    def directives(self):
        return self._o.response.directives

    def get_directive(self, name):
        for d in self.directives:
            if d.name == name:
                return d

    @cached_property
    def scenario_analytics_info(self):
        _scenario = self.scenario or scenario.AliceVins
        scenario_info = self._o.megamind_analytics_info.analytics_info.get(_scenario, {})
        return MegamindAnalyticsInfo(scenario_info)

    @property
    def modifiers_analytics_info(self):
        return self._o.megamind_analytics_info.modifiers_analytics_info

    @property
    def megamind_modifiers_info(self):
        return self._o.megamind_analytics_info.modifiers_info

    @cached_property
    def slots(self):
        return {
            _.name: (_.typed_value if hasattr(_, 'typed_value') else _)
            for _ in self.scenario_analytics_info.slots
        }

    @cached_property
    def suggests(self):
        return self._o.response.get('suggest', {}).get('items', [])

    def suggest(self, title):
        for s in self.suggests:
            if s.title == title:
                return s

    @property
    def quality_storage(self):
        return self._o.response.quality_storage

    @property
    def cards(self):
        return self._o.response.cards

    @cached_property
    def div_cards(self):
        return [DivCard(c) for c in self.cards if c.type in div_card_types]

    @cached_property
    def div_card(self):
        return self.div_cards[0] if self.div_cards else None

    @cached_property
    def text_cards(self):
        return [c for c in self.cards if c.type in {'simple_text', 'text_with_button'}]

    @cached_property
    def text_card(self):
        return self.text_cards[0] if self.text_cards else None

    @property
    def proactivity_log_storage_actions(self):
        return self._o.proactivity_log_storage.get('actions', [])

    @property
    def raw(self):
        return self._o.response
