# -*- coding: utf-8 -*-

import attr
import json
import logging

logger = logging.getLogger(__name__)


UNDEFINED_INDEX = -1
MAX_GALLERY_SIZE = 15


@attr.s(frozen=True)
class SimilarMovieInfo(object):
    onto_id = attr.ib()
    similarity = attr.ib()


@attr.s
class MovieInfo(object):
    name = attr.ib()
    kinopoisk_id = attr.ib()
    min_age = attr.ib()
    content_type = attr.ib()
    genres = attr.ib()
    description = attr.ib()
    kinopoisk_url = attr.ib()
    cover_url = attr.ib()
    rating = attr.ib()
    popularity = attr.ib(default=None)
    onto_id = attr.ib(default=None)
    similar_movie_infos = attr.ib(default=None)


@attr.s
class Node(object):
    index = attr.ib()
    movie_info_index = attr.ib(default=UNDEFINED_INDEX)
    left_child_index = attr.ib(default=UNDEFINED_INDEX)
    right_child_index = attr.ib(default=UNDEFINED_INDEX)
    subtree_size = attr.ib(default=1)
    left_image_url = attr.ib(default=None)
    right_image_url = attr.ib(default=None)
    movie_infos = attr.ib(default=None)


@attr.s
class Tree(object):
    nodes = attr.ib()
    filter_info = attr.ib()
    content_name = attr.ib()


def load_movie_infos(path):
    movie_infos = []
    with open(path) as f:
        json_data = json.load(f)

        for movie_info_json in json_data:
            movie_info = MovieInfo(**movie_info_json)
            movie_info.similar_movie_infos = [
                SimilarMovieInfo(**similar_movie_info) for similar_movie_info in movie_info.similar_movie_infos
            ]

            movie_infos.append(movie_info)

    logger.info('Movie info count: %s', len(movie_infos))
    logger.info('Example: %s', movie_infos[0])

    return movie_infos
