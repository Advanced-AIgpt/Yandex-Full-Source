import argparse
import codecs
import json

from vins_core.common.sample import Sample
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.registry import create_sample_processor
from vins_core.ext.entitysearch import EntitySearchHTTPAPI


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--entity_base_filename', required=True)
    parser.add_argument('--phrase_filename', required=True, help='File with phrases from which '
                                                                 'entities should be extracted')
    args = parser.parse_args()
    with codecs.open(args.entity_base_filename, encoding='utf-8') as entity_base_file:
        current_entity_base = json.load(entity_base_file) or {}

    entitysearch = EntitySearchHTTPAPI()
    samples_extractor = SamplesExtractor(pipeline=[create_sample_processor('wizard')])
    with codecs.open(args.phrase_filename, encoding='utf-8') as phrase_file:
        for phrase in phrase_file:
            sample = Sample.from_string(phrase)
            sample = samples_extractor([sample])[0]
            entity_finder_rule = sample.annotations['wizard'].rules['EntityFinder']
            winners = entity_finder_rule.get('Winner', [])
            if isinstance(winners, basestring):
                winners = [winners]

            entity_ids = [winner.split('\t')[3] for winner in winners]
            extracted_entities = entitysearch.get_response(entity_ids)
            current_entity_base.update(extracted_entities)
    with codecs.open(args.entity_base_filename, 'w', encoding='utf-8') as entity_base_file:
        json.dump(current_entity_base, entity_base_file, ensure_ascii=False)


if __name__ == '__main__':
    main()
