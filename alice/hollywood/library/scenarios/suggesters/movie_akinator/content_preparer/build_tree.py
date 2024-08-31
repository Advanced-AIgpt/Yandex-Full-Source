# -*- coding: utf-8 -*-

import argparse
import attr
import itertools
import json
import logging
import numpy as np

from sknetwork.hierarchy import Paris

from common import SimilarMovieInfo, MovieInfo, Node, load_movie_infos, MAX_GALLERY_SIZE, UNDEFINED_INDEX

logger = logging.getLogger(__name__)


_CONTENT_NAMES = {
    (None, None): 'кино',
    ('tv_show', None): 'сериалы',
    ('cartoon', None): 'мультики',
    ('movie', None): 'фильмы',
    ('tv_show', 'detective'): 'детективные сериалы',
    ('tv_show', 'drama'): 'драматические сериалы',
    ('tv_show', 'crime'): 'криминальные сериалы',
    ('tv_show', 'melodramas'): 'мелодраматические сериалы',
    ('tv_show', 'thriller'): 'сериалы-триллеры',
    ('cartoon', 'comedy'): 'смешные мультики',
    ('cartoon', 'adventure'): 'приключенческие мультики',
    ('cartoon', 'fantasy'): 'фантастические мультики',
    ('cartoon', 'childrens'): 'мультики для детей',
    ('cartoon', 'family'): 'семейные мультики',
    ('movie', 'comedy'): 'комедии',
    ('movie', 'detective'): 'детективы',
    ('movie', 'science_fiction'): 'научную фантастику',
    ('movie', 'drama'): 'драмы',
    ('movie', 'crime'): 'криминальное кино',
    ('movie', 'horror'): 'ужасы',
    ('movie', 'historical'): 'исторические фильмы',
    ('movie', 'adventure'): 'приключения',
    ('movie', 'fantasy'): 'фэнтези',
    ('movie', 'action'): 'боевики',
    ('movie', 'family'): 'семейное кино',
    ('movie', 'melodramas'): 'мелодрамы',
    ('movie', 'thriller'): 'триллеры',
    ('movie', 'biopic'): 'биографические фильмы',
    ('movie', 'musical'): 'мюзикл',
}


def _print_tree(node, nodes, movie_infos, indent):
    if node.movie_info_index != UNDEFINED_INDEX:
        print(' ' * indent, movie_infos[node.movie_info_index].name)
        return

    if node.left_child_index != UNDEFINED_INDEX:
        _print_tree(nodes[node.left_child_index], nodes, movie_infos, indent + 1)
    if node.right_child_index != UNDEFINED_INDEX:
        _print_tree(nodes[node.right_child_index], nodes, movie_infos, indent + 1)


def _print_large_clusters(node, nodes, movie_infos, min_cluster_size=50):
    if node.subtree_size <= min_cluster_size:
        movie_infos = sorted(node.movie_infos, key=lambda movie_info: movie_info.popularity, reverse=True)
        logger.info('; '.join(movie_info.name for movie_info in movie_infos))
        return

    if node.left_child_index != UNDEFINED_INDEX:
        _print_large_clusters(nodes[node.left_child_index], nodes, movie_infos, min_cluster_size)
    if node.right_child_index != UNDEFINED_INDEX:
        _print_large_clusters(nodes[node.right_child_index], nodes, movie_infos, min_cluster_size)


def _build_adjacency_matrix(movie_infos, onto_id_to_movie_info_index):
    logger.info('Building adjacency matrix')

    adjacency_matrix = np.zeros((len(onto_id_to_movie_info_index), len(onto_id_to_movie_info_index)))

    for onto_id, movie_info_index in onto_id_to_movie_info_index.items():
        movie_info = movie_infos[movie_info_index]

        for similar_movie_info in movie_info.similar_movie_infos:
            similar_movie_info_index = onto_id_to_movie_info_index.get(similar_movie_info.onto_id)
            if not similar_movie_info_index:
                continue

            adjacency_matrix[movie_info_index, similar_movie_info_index] = similar_movie_info.similarity

    return adjacency_matrix


def _build_dendrogram(adjacency_matrix):
    logger.info('Building dendrogram')

    paris = Paris(weights='degree', reorder=False)
    return paris.fit_transform(adjacency_matrix)


def _build_clustering_tree(filtered_movie_info_indices_reverse_mapping, dendrogram):
    def _append_leaf(index):
        index_to_child[index] = Node(
            index=index,
            movie_info_index=filtered_movie_info_indices_reverse_mapping[index]
        )

    logger.info('Building tree')

    filtered_movie_info_count = len(filtered_movie_info_indices_reverse_mapping)

    index_to_child = {}
    for result_index, (from_index, to_index, _, size) in enumerate(dendrogram):
        from_index, to_index, size = int(from_index), int(to_index), int(size)
        result_index += filtered_movie_info_count

        if from_index < filtered_movie_info_count:
            _append_leaf(from_index)
        if to_index < filtered_movie_info_count:
            _append_leaf(to_index)

        index_to_child[result_index] = Node(
            left_child_index=from_index,
            right_child_index=to_index,
            index=result_index,
            subtree_size=index_to_child[from_index].subtree_size + index_to_child[to_index].subtree_size
        )
        assert index_to_child[result_index].subtree_size == size

    assert index_to_child[result_index].subtree_size == filtered_movie_info_count

    nodes = []
    for index, node in sorted(index_to_child.items(), key=lambda pair: pair[0]):
        assert len(nodes) == index
        nodes.append(node)

    return nodes


def _build_hierarchical_clustering(movie_infos, filtered_movie_info_indices_reverse_mapping):
    onto_id_to_movie_info_index = {movie_info.onto_id: index for index, movie_info in enumerate(movie_infos)}

    adjacency_matrix = _build_adjacency_matrix(movie_infos, onto_id_to_movie_info_index)
    dendrogram = _build_dendrogram(adjacency_matrix)
    nodes = _build_clustering_tree(filtered_movie_info_indices_reverse_mapping, dendrogram)

    logger.info('Collected %s nodes', len(nodes))

    return nodes


def _collect_filtered_movie_infos(movie_infos, filter_condition):
    filtered_movie_infos, filtered_movie_info_indices_reverse_mapping = [], []
    for movie_info_index, movie_info in enumerate(movie_infos):
        if filter_condition(movie_info):
            filtered_movie_infos.append(movie_info)
            filtered_movie_info_indices_reverse_mapping.append(movie_info_index)
    return filtered_movie_infos, filtered_movie_info_indices_reverse_mapping


def _run_clustering(movie_infos, filter_info, filter_condition):
    filtered_movie_infos, indices_reverse_mapping = _collect_filtered_movie_infos(movie_infos, filter_condition)

    logger.info('Number of movies that satisfies the filter condition = %s', len(filtered_movie_infos))
    if len(filtered_movie_infos) < MAX_GALLERY_SIZE:
        return

    tree_nodes = _build_hierarchical_clustering(filtered_movie_infos, indices_reverse_mapping)
    return filter_info, tree_nodes


def _iterate_filtered_trees(movie_infos):
    logger.info('Collecting unfiltered tree')
    tree_nodes = _build_hierarchical_clustering(movie_infos, list(range(len(movie_infos))))
    yield {}, tree_nodes

    content_types, genres = set(), set()
    for movie_info in movie_infos:
        content_types.add(movie_info.content_type)
        genres.update(movie_info.genres)

    for content_type in content_types:
        logger.info('Collecting tree w/ content_type = %s', content_type)
        clustering_result = _run_clustering(
            movie_infos=movie_infos,
            filter_info={'content_type': content_type},
            filter_condition=lambda movie_info: movie_info.content_type == content_type
        )
        if clustering_result:
            yield clustering_result

    for content_type, genre in itertools.product(content_types, genres):
        logger.info('Collecting tree w/ content_type = %s, genre = %s', content_type, genre)
        clustering_result = _run_clustering(
            movie_infos=movie_infos,
            filter_info={'content_type': content_type, 'genre': genre},
            filter_condition=lambda movie_info: genre in movie_info.genres and movie_info.content_type == content_type
        )
        if clustering_result:
            yield clustering_result


def _build_trees(movie_infos_path, output_path):
    movie_infos = load_movie_infos(movie_infos_path)
    movie_infos = sorted(movie_infos, key=lambda movie_info: movie_info.popularity, reverse=True)

    trees = []
    for movie_infos_filter, nodes in _iterate_filtered_trees(movie_infos):
        nodes = [attr.asdict(node) for node in nodes]
        for node in nodes:
            del node['movie_infos']

        content = (movie_infos_filter.get('content_type'), movie_infos_filter.get('genre'))
        trees.append({
            'filter': movie_infos_filter,
            'tree_nodes': nodes,
            'content_name': _CONTENT_NAMES.get(content)
        })

    movie_infos = [attr.asdict(movie_info) for movie_info in movie_infos]

    for movie_info in movie_infos:
        del movie_info['similar_movie_infos']

    result = {
        'movie_infos': movie_infos,
        'trees': trees
    }
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(result, f, ensure_ascii=False, indent=2)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--movie-infos-path', required=True, help='Path to movie_infos.json')
    parser.add_argument('--result-path', required=True, help='Path to result json')
    args = parser.parse_args()

    _build_trees(args.movie_infos_path, args.result_path)


if __name__ == "__main__":
    main()
