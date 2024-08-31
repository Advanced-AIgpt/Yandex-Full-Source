import json
import logging
from vins_core.common.entity import Entity
from vins_core.utils.sequence_alignment import align_sequences


logger = logging.getLogger(__name__)


class NluWizardAliceTypeParserTime(object):
    RULE_NAME = 'AliceTypeParserTime'

    def __init__(self, **kwargs):
        self.fst_name = 'wizard_time'

    def _parse_impl(self, sample):
        wizard = sample.annotations.get('wizard')
        entities = []
        if not wizard or not wizard.rules.get(self.RULE_NAME):
            return entities

        rule_result = wizard.rules[self.RULE_NAME].get('Result', None)
        if not rule_result:
            return entities

        parsed_entities_by_type = rule_result.get('ParsedEntitiesByType', None)
        tokens = rule_result.get('Tokens')
        if not parsed_entities_by_type or not tokens:
            return entities

        alignment = align_sequences(tokens, sample.tokens)

        for entities_with_type in parsed_entities_by_type:
            type_key = entities_with_type['key']
            parsed_entities = entities_with_type['value']
            if type_key != 'time':
                continue

            for entity in parsed_entities.get('ParsedEntities', []):
                unaligned_begin = entity['StartToken']
                begin = alignment[unaligned_begin] if 0 <= unaligned_begin < len(alignment) else -1

                unaligned_end = entity['EndToken']
                end = alignment[unaligned_end - 1] + 1 if 0 <= unaligned_end - 1 < len(alignment) else -1

                entity_type = entity['Type'].upper()
                value = json.loads(entity['Value'])
                text = entity['Text']

                if begin < 0 or end < 1:
                    logger.warning('Wizard markup error: unable to align value %r at range %r', value, (begin, end))
                    logger.warning('Token alignment: %r', alignment)
                    continue

                entities.append(Entity(
                    start=begin,
                    end=end,
                    type=entity_type,
                    value=value,
                    substr=text
                ))

            break

        return entities

    def parse(self, sample):
        try:
            return self._parse_impl(sample)
        except KeyError:
            return []

    def __call__(self, sample, *args, **kwargs):
        return self.parse(sample)
