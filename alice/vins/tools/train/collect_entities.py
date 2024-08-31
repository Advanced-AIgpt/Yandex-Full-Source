# -*- coding: utf-8 -*-

import click
import time

from itertools import izip
from requests.exceptions import ConnectionError

from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.common.sample import Sample
from vins_core.ext.entitysearch import EntitySearchHTTPAPI
from vins_core.utils.misc import parallel

from dataset import VinsDataset


def _get_entitysearch_response(entitysearch_api, winner_ids, retries_count):
    for _ in xrange(retries_count):
        try:
            return entitysearch_api.get_response(winner_ids)
        except ConnectionError:
            time.sleep(1)

    return {}


def _get_entity_search_result(utterance, samples_extractor, entitysearch_api, retries_count, **kwargs):
    sample = Sample.from_string(utterance)

    sample_features = samples_extractor([sample])[0]
    entity_search_results = {}
    if 'wizard' in sample_features.annotations:
        rules = sample_features.annotations['wizard'].rules

        entity_finder_winners = rules['EntityFinder'].get('Winner', [])
        if isinstance(entity_finder_winners, basestring):
            entity_finder_winners = [entity_finder_winners]

        winner_ids = {winner.split('\t')[3] for winner in entity_finder_winners}
        entity_search_results = _get_entitysearch_response(entitysearch_api, winner_ids, retries_count)

    entity_search_result = set()

    entity_search_result.update(
        ('tag_' + tag for res in entity_search_results.itervalues() for tag in res.get('tags', []))
    )

    base_infos = [res['base_info'] for res in entity_search_results.itervalues()]

    entity_search_result.update(
        ('id_' + id for base_info in base_infos for id in base_info.get('ids', {}).keys())
    )
    entity_search_result.update(
        ('type_' + base_info['type'] for base_info in base_infos if 'type' in base_info)
    )
    entity_search_result.update(
        ('subtype_' + subtype for base_info in base_infos for subtype in base_info.get('wsubtype', ()))
    )
    if any('music_info' in base_info for base_info in base_infos):
        entity_search_result.add('music_info')

    return entity_search_result


@click.command()
@click.option('--data-path', type=click.Path(exists=True))
@click.option('--retries-count', default=5)
def main(data_path, retries_count):
    samples_extractor = SamplesExtractor.from_config({
        "pipeline": [
            {"name": "wizard"},
            {"name": "entitysearch"}
        ]
    })

    entitysearch_api = EntitySearchHTTPAPI()

    dataset = VinsDataset.restore(data_path)

    entity_search_results = parallel(
        function=_get_entity_search_result,
        items=dataset._preprocessed_texts,
        function_kwargs={
            'samples_extractor': samples_extractor,
            'entitysearch_api': entitysearch_api,
            'retries_count': retries_count
        },
        filter_errors=False,
        raise_on_error=True
    )

    for info, entity_search_result in izip(dataset._additional_infos, entity_search_results):
        info.entities_info = entity_search_result

    dataset.save(data_path)


if __name__ == '__main__':
    main()
