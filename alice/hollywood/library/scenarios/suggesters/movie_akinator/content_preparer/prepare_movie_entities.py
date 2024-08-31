# -*- coding: utf-8 -*-

import argparse
import json
import logging
import re

from collections import Counter
from string import punctuation

from common import MovieInfo, load_movie_infos

logger = logging.getLogger(__name__)


def _preprocess_movie_name(movie_name):
    movie_name_tokens = re.split('[ {}–—«»]'.format(punctuation), movie_name)
    movie_name_tokens = filter(lambda token: token, movie_name_tokens)
    return ' '.join(movie_name_tokens)


def _dump_movies(movie_infos, indent, out_file):
    for movie_info in movie_infos:
        if '{' in movie_info.name:
            continue

        out_file.write(indent + '%value "{}"\n'.format(movie_info.onto_id))
        out_file.write(indent + '{}\n\n'.format(_preprocess_movie_name(movie_info.name)))


def _collect_movie_series_names(movie_infos):
    maybe_series_name = Counter()

    for movie_info in movie_infos:
        split_movie_info = re.split('(:?:|(:?Фильм | Часть |, глава |№)?\d)', movie_info.name, re.IGNORECASE)
        if len(split_movie_info) > 1 and split_movie_info[0].strip():
            maybe_series_name[split_movie_info[0].strip()] += 1

    movie_series = {name for name, count in maybe_series_name.most_common() if count > 1}

    movie_series.update({
        'Гарри Поттер',
        'Стражи Галактики',
        'Темный рыцарь'
    })

    return movie_series


def _dump_movie_series(movie_infos, indent, out_file):
    movie_series = _collect_movie_series_names(movie_infos)

    for name in movie_series:
        for movie_info in movie_infos:
            if movie_info.name.startswith(name):
                out_file.write(indent + '%value "{}"\n'.format(movie_info.onto_id))
                out_file.write(indent + '{}\n\n'.format(_preprocess_movie_name(name)))
                break


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--movie-infos-path', required=True, help='Path to movie_infos.json')
    parser.add_argument('--output-path', required=True, help='Path to movie_entities.grnt')
    args = parser.parse_args()

    movie_infos = load_movie_infos(args.movie_infos_path)
    with open(args.output_path, 'w') as f:
        f.write('$MovieEntity:\n')

        indent = ' ' * 4
        f.write(indent + '%lemma\n\n')
        f.write(indent + '%type "entity_search.film"\n\n')

        _dump_movies(movie_infos, indent, out_file=f)
        _dump_movie_series(movie_infos, indent, out_file=f)


if __name__ == "__main__":
    main()
