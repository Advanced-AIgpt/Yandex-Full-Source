# coding: utf-8

import argparse
import json
import os
import yt.wrapper as yt
import yaml
import logging

from collections import defaultdict
from tqdm import tqdm

logger = logging.getLogger(__name__)


def load_known_movies(table_path):
    entity_id_to_questions = defaultdict(lambda: defaultdict(list))

    for row in tqdm(yt.read_table(table_path, yt.YsonFormat(encoding=None))):
        entity_id = row[b'entity_id'].decode('utf8')
        entity_id, is_positive = list(map(int, entity_id.split('_')))

        question = row[b'context_0'].decode('utf8') + '?'
        entity_id_to_questions[entity_id][is_positive].append(question)

    entity_id_to_info = {}
    for entity_id, questions_info in entity_id_to_questions.items():
        negatives_count = len(questions_info.get(0, []))
        total_count = negatives_count + len(questions_info.get(1, []))

        entity_id_to_info[entity_id] = {
            'positives': list(set(questions_info.get(1, []))),
            'negatives': list(set(questions_info.get(0, []))),
            'negatives_fraction': negatives_count * 1. / total_count,
        }

    return entity_id_to_info


def load_id_to_movie_info():
    id_to_movie_info = {}
    for row in yt.read_table('//home/voice/dan-anastasev/kinopoisk_to_onto_ids', yt.YsonFormat(encoding=None)):
        movie_id = row[b'kinopoisk_id']
        if movie_id not in id_to_movie_info:
            id_to_movie_info[movie_id] = {
                'id': movie_id,
                'type': row[b'content_type'].decode('utf8'),
                'title': row[b'title'].decode('utf8')
            }

    return id_to_movie_info


def build_known_movies(entity_id_to_info, id_to_movie_info):
    known_movies = []
    for entity_id, questions_info in entity_id_to_info.items():
        if entity_id not in id_to_movie_info:
            logger.warning('Entity %s was not found', entity_id)
            continue

        movie_info = id_to_movie_info[entity_id]
        movie_info['questions'] = questions_info
        known_movies.append(movie_info)

    return known_movies


def main():
    yt.config['proxy']['url'] = 'hahn'
    yt.config['read_parallel']['enable'] = True
    yt.config['read_parallel']['max_thread_count'] = 32

    parser = argparse.ArgumentParser()
    parser.add_argument('--shard-config-path',
                        default=os.path.join(os.getenv('ARCADIA'), 'alice/boltalka/extsearch/shard/data.yaml'))
    args = parser.parse_args()

    with open(args.shard_config_path) as f:
        config = yaml.load(f)

    entity_id_to_info = None
    for index_info in config['entity_indexes']:
        if index_info['name'] == 'movie':
            entity_id_to_info = load_known_movies(index_info['table'])

    assert entity_id_to_info is not None

    id_to_movie_info = load_id_to_movie_info()

    known_movies = build_known_movies(entity_id_to_info, id_to_movie_info)

    with open('known_movies.json', 'w') as f:
        for movie_info in known_movies:
            f.write(json.dumps(movie_info, ensure_ascii=False) + '\n')


if __name__ == '__main__':
    main()
