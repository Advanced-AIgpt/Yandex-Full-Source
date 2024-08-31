import json
from vins_core.common.entity import Entity


class NluFstWizard(object):
    @staticmethod
    def parse_rule_result(raw_rule_result):
        entities = []
        if len(raw_rule_result) == 0:
            return entities
        for raw_entity in raw_rule_result['entities']:
            entity = json.loads(raw_entity)
            assert len(entity['value']) == 1, 'A rule result contains more than 1 value'
            entity['value'] = entity['value'].values()[0]
            entities.append(Entity(**entity))

        return entities

    @staticmethod
    def create_rule_name(fst_name):
        return 'Fst' + fst_name.capitalize()

    def __init__(self, fst_name, **kwargs):
        self.fst_name = fst_name
        self._rule_name = NluFstWizard.create_rule_name(fst_name)

    def parse(self, sample):
        wizard = sample.annotations.get('wizard')
        if wizard and self._rule_name in wizard.rules:
            return NluFstWizard.parse_rule_result(wizard.rules[self._rule_name])

        return []

    def __call__(self, sample, *args, **kwargs):
        return self.parse(sample)
