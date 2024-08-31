# -*- coding: utf-8 -*-

import argparse
import attr
import io
import json
import logging
import requests
import urllib
import uuid

from collections import defaultdict
from multiprocessing import Pool
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from tqdm import tqdm

from common import SimilarMovieInfo, MovieInfo, Node, Tree, UNDEFINED_INDEX, MAX_GALLERY_SIZE

logger = logging.getLogger(__name__)


_IMAGE_WIDTH, _IMAGE_HEIGHT = 328, 492
_IMAGE_PADDING = 60

_SMALL_IMAGE_WIDTH, _SMALL_IMAGE_HEIGHT = _IMAGE_WIDTH, _IMAGE_HEIGHT
_BIG_IMAGE_WIDTH, _BIG_IMAGE_HEIGHT = 5 * _SMALL_IMAGE_WIDTH // 4, 5 * _SMALL_IMAGE_HEIGHT // 4

_CLOUD_WIDTH, _CLOUD_HEIGHT = 1152, 2016
_SMALL_CLOUD_WIDTH, _SMALL_CLOUD_HEIGHT = 900, 1575
_CLOUD_PADDING = 30


def _get_ordered_priorities(priority_grid):
    ordered_priorities = [-1] * sum(len(priorities) for priorities in priority_grid)
    index = 0
    for priorities in priority_grid:
        for priority in priorities:
            assert ordered_priorities[priority] == -1, '{} {}'.format(priority, index)

            ordered_priorities[priority] = index
            index += 1

    return ordered_priorities


_GRID_PRIORITIES = _get_ordered_priorities([
    [16, 7, 6, 8, 13],
    [12, 2, 0, 5, 14],
    [3, 1, 4, 17],
    [10, 9, 11, 15],
])


_AVATARS_READ_URL = 'http://avatars.mds.yandex.net/get-bass'
_AVATARS_WRITE_URL = 'http://avatars-int.mds.yandex.net:13000/put-bass'


@attr.s
class GridElement(object):
    image = attr.ib()
    image_width = attr.ib()
    image_height = attr.ib()
    box = attr.ib()


def _fill_movie_infos(movie_infos, nodes):
    for node in nodes:
        if node.movie_info_index != UNDEFINED_INDEX:
            node.movie_infos = [movie_infos[node.movie_info_index]]
        else:
            assert node.left_child_index != UNDEFINED_INDEX
            assert node.right_child_index != UNDEFINED_INDEX

            left_child, right_child = nodes[node.left_child_index], nodes[node.right_child_index]
            assert left_child.movie_infos and right_child.movie_infos

            node.movie_infos = left_child.movie_infos + right_child.movie_infos


def _load_data(path):
    with open(path) as f:
        data = json.load(f)

    movie_infos = [MovieInfo(**movie_info_json) for movie_info_json in data['movie_infos']]

    trees = []
    for tree_json in data['trees']:
        nodes = [Node(**node_json) for node_json in tree_json['tree_nodes']]
        _fill_movie_infos(movie_infos, nodes)

        filter_info = tree_json['filter']
        content_name = tree_json['content_name']
        trees.append(Tree(nodes, filter_info, content_name))

    return movie_infos, trees


def _add_corners(image, radius):
    circle = Image.new('L', (radius * 2, radius * 2), 0)
    draw = ImageDraw.Draw(circle)
    draw.ellipse((0, 0, radius * 2, radius * 2), fill=255)

    alpha = Image.new('L', image.size, 'white')
    w, h = image.size

    alpha.paste(circle.crop((0, 0, radius, radius)), (0, 0))
    alpha.paste(circle.crop((0, radius, radius, radius * 2)), (0, h - radius))
    alpha.paste(circle.crop((radius, 0, radius * 2, radius)), (w - radius, 0))
    alpha.paste(circle.crop((radius, radius, radius * 2, radius * 2)), (w - radius, h - radius))

    image.putalpha(alpha)
    return image


def _make_shadow(image, iterations, border, offset, background_color, shadow_color):
    full_width = image.size[0] + abs(offset[0]) + 2 * border[0]
    full_height = image.size[1] + abs(offset[1]) + 2 * border[1]

    shadow = Image.new(image.mode, (full_width, full_height), background_color)

    shadow_left = border[0] + max(offset[0], 0)
    shadow_top = border[1] + max(offset[1], 0)

    shadow_box = [shadow_left, shadow_top, shadow_left + image.size[0], shadow_top + image.size[1]]
    shadow.paste(shadow_color, box=shadow_box)

    for i in range(iterations):
        shadow = shadow.filter(ImageFilter.BLUR)

    image_left = border[0] - min(offset[0], 0)
    image_top = border[1] - min(offset[1], 0)
    shadow.paste(image, (image_left, image_top), image)

    return shadow


def _prepare_grid_for_12_plus_elements():
    grid = []
    height = _IMAGE_PADDING
    for row_index in range(4):
        if row_index < 2:
            images_in_row = 5
            _IMAGE_WIDTH, _IMAGE_HEIGHT = _SMALL_IMAGE_WIDTH, _SMALL_IMAGE_HEIGHT
        else:
            images_in_row = 4
            _IMAGE_WIDTH, _IMAGE_HEIGHT = _BIG_IMAGE_WIDTH, _BIG_IMAGE_HEIGHT

        for image_index in range(images_in_row):
            grid.append(GridElement(
                image=None,
                box=(image_index * (_IMAGE_WIDTH + _IMAGE_PADDING) + _IMAGE_PADDING, height),
                image_width=_IMAGE_WIDTH,
                image_height=_IMAGE_HEIGHT
            ))

        height += _IMAGE_HEIGHT + _IMAGE_PADDING

    canvas_width = 5 * _SMALL_IMAGE_WIDTH + 6 * _IMAGE_PADDING
    canvas_height = 2 * _SMALL_IMAGE_HEIGHT + 2 * _BIG_IMAGE_HEIGHT + 5 * _IMAGE_PADDING

    priorities = _GRID_PRIORITIES

    left, top = 240, 250
    box = (left, top, left + _CLOUD_WIDTH, top + _CLOUD_HEIGHT)

    return grid, priorities, (canvas_width, canvas_height), box


def _prepare_grid_for_4_plus_elements():
    grid, priorities, canvas_size, _ = _prepare_grid_for_12_plus_elements()

    left, top = 300, 300
    box = (left, top, left + _SMALL_CLOUD_WIDTH, top + _SMALL_CLOUD_HEIGHT)
    return grid, priorities, canvas_size, box


def _prepare_grid_for_less_then_4_elements(count):
    assert 1 <= count <= 3

    _IMAGE_WIDTH, _IMAGE_HEIGHT = _BIG_IMAGE_WIDTH, _BIG_IMAGE_HEIGHT
    grid = []
    for image_index in range(count):
        grid.append(GridElement(
            image=None,
            box=(_IMAGE_PADDING + image_index * (_IMAGE_WIDTH + _IMAGE_PADDING), _IMAGE_PADDING),
            image_width=_IMAGE_WIDTH,
            image_height=_IMAGE_HEIGHT
        ))

    if count == 3:
        priorities = [1, 0, 2]
    elif count == 2:
        priorities = [0, 1]
    else:
        priorities = [0]

    canvas_width = (_IMAGE_WIDTH + _IMAGE_PADDING) * count + 3 * _IMAGE_PADDING
    canvas_height = _IMAGE_HEIGHT + 4 * _IMAGE_PADDING

    return grid, priorities, (canvas_width, canvas_height), None


def _generate_covers_grid(movie_infos):
    grid_generator = None
    if len(movie_infos) >= 12:
        grid_generator = _prepare_grid_for_12_plus_elements
    elif len(movie_infos) >= 4:
        grid_generator = _prepare_grid_for_4_plus_elements
    else:
        def grid_generator():
            return _prepare_grid_for_less_then_4_elements(len(movie_infos))

    grid, priorities, canvas_size, rotated_box = grid_generator()

    for movie_info, priority in zip(movie_infos, priorities):
        image = Image.open(urllib.request.urlopen(movie_info.cover_url)).convert('RGBA')
        image = _add_corners(image, radius=20)
        grid[priority].image = image.resize((grid[priority].image_width, grid[priority].image_height))

    return grid, canvas_size, rotated_box


def _render_enlarged_main_image(main_image, canvas):
    main_image = main_image.resize((int(_BIG_IMAGE_WIDTH * 1.2), int(_BIG_IMAGE_HEIGHT * 1.2)))
    main_image = _make_shadow(main_image, iterations=200, border=(100, 50), offset=(-50, 25),
                                background_color=(0, 0, 0, 0), shadow_color=(255, 255, 255, 128))
    canvas.paste(
        main_image,
        box=(2 * _SMALL_IMAGE_WIDTH + _IMAGE_PADDING - 100, _SMALL_IMAGE_HEIGHT + _IMAGE_PADDING // 2 - 50),
        mask=main_image
    )


def _render_grid(grid, canvas_size):
    canvas = Image.new('RGBA', canvas_size, color = (0,) * 4)

    for element in grid:
        if element.image:
            canvas.paste(element.image, box=element.box)

    main_image_index = _GRID_PRIORITIES[0]
    if len(grid) > main_image_index:
        _render_enlarged_main_image(grid[main_image_index].image, canvas)

    return canvas


def _rotate_gallery(canvas, canvas_size, rotated_box):
    canvas_width, canvas_height = canvas_size

    canvas = canvas.rotate(20, fillcolor=(0,) * 4, expand=rotated_box is None)
    if rotated_box:
        canvas = canvas.crop(box=(rotated_box))
        return canvas.resize((_CLOUD_WIDTH, _CLOUD_HEIGHT))

    new_canvas = Image.new('RGBA', (_CLOUD_WIDTH, _CLOUD_HEIGHT), color = (0,) * 4)
    box = ((_CLOUD_WIDTH - canvas_width) // 2, (_CLOUD_HEIGHT - canvas_height) // 2)
    new_canvas.paste(canvas, box=box)
    return new_canvas


def _move_poster_cloud(canvas, is_left):
    new_canvas = Image.new('RGBA', (_CLOUD_WIDTH + _CLOUD_PADDING, _CLOUD_HEIGHT), color=(255,) * 4)
    new_canvas.paste(canvas, box=(0 if is_left else _CLOUD_PADDING, 0), mask=canvas)
    return new_canvas


def _render_poster_cloud(movie_infos):
    grid, canvas_size, rotated_box = _generate_covers_grid(movie_infos)

    canvas = _render_grid(grid, canvas_size)
    canvas = _rotate_gallery(canvas, canvas_size, rotated_box)

    canvas = _add_corners(canvas, radius=45)
    left_canvas = _move_poster_cloud(canvas, is_left=True)
    right_canvas = _move_poster_cloud(canvas, is_left=False)

    target_size = (left_canvas.size[0] // 4, left_canvas.size[1] // 4)
    left_canvas = left_canvas.resize(target_size, resample=Image.LANCZOS)
    right_canvas = right_canvas.resize(target_size, resample=Image.LANCZOS)

    return left_canvas.convert('RGB'), right_canvas.convert('RGB')


def _upload_image(raw_image, content_type='image/png'):
    upload_name = str(uuid.uuid4())

    with io.BytesIO() as image:
        raw_image.save(image, format='PNG')
        image.seek(0)

        upload_url = '{}/{}'.format(_AVATARS_WRITE_URL, upload_name)
        response = requests.post(
            upload_url,
            files=[('file', ('file', image, content_type))]
        )

    if response.status_code != 200:
        logger.error('Failed to upload image: %s returned code %d with text\n%s',
                     upload_url, response.status_code, response.text)
        raise RuntimeError('Failed to upload image')

    response_json = response.json()

    group = response_json['group-id']
    avatars_image_name = response_json['imagename']
    return '{}/{}/{}/orig'.format(_AVATARS_READ_URL, group, avatars_image_name)


def _generate_cluster_image(movie_infos):
    left_cluster_image, right_cluster_image = _render_poster_cloud(movie_infos)
    return _upload_image(left_cluster_image), _upload_image(right_cluster_image)


def _iterate_clusters_to_render(tree):
    queue = [tree.nodes[-1]]
    while queue:
        node = queue.pop(0)

        yield node

        if node.subtree_size <= MAX_GALLERY_SIZE:
            continue

        if node.left_child_index != UNDEFINED_INDEX:
            queue.append(tree.nodes[node.left_child_index])

        if node.right_child_index != UNDEFINED_INDEX:
            queue.append(tree.nodes[node.right_child_index])


def _deduplicate_clusters(clusters_to_render):
    def _get_visible_items(movie_infos):
        movie_infos = sorted(movie_infos, key=lambda movie_info: movie_info.popularity, reverse=True)
        return movie_infos[:len(_GRID_PRIORITIES)]

    def _get_movie_info_ids(movie_infos):
        return tuple(sorted(movie_info.kinopoisk_id for movie_info in movie_infos))

    unique_movie_infos = []
    movie_info_ids_to_cluster_indices = defaultdict(list)
    for cluster_index, node in enumerate(clusters_to_render):
        movie_infos = _get_visible_items(node.movie_infos)

        movie_info_ids = _get_movie_info_ids(movie_infos)
        same_movie_cluster_indices = movie_info_ids_to_cluster_indices[movie_info_ids]

        if not same_movie_cluster_indices:
            unique_movie_infos.append(movie_infos)

        same_movie_cluster_indices.append(cluster_index)

    unique_movie_infos_cluster_indices = []
    for movie_infos in unique_movie_infos:
        movie_info_ids = _get_movie_info_ids(movie_infos)
        same_movie_cluster_indices = movie_info_ids_to_cluster_indices[movie_info_ids]
        unique_movie_infos_cluster_indices.append(same_movie_cluster_indices)

        unique_movie_info_ids = set()
        for cluster_index in same_movie_cluster_indices:
            movie_infos = _get_visible_items(clusters_to_render[cluster_index].movie_infos)
            unique_movie_info_ids.add(_get_movie_info_ids(movie_infos))

        assert len(unique_movie_info_ids) == 1

    return unique_movie_infos, unique_movie_infos_cluster_indices


def _render_clusters(trees):
    clusters_to_render = [node for tree in trees for node in _iterate_clusters_to_render(tree)]

    unique_movie_infos, unique_movie_infos_cluster_indices = _deduplicate_clusters(clusters_to_render)

    with Pool(processes=64) as pool:
        image_iterator = pool.imap(_generate_cluster_image, unique_movie_infos)

        for unique_movie_info_index, image_urls in enumerate(tqdm(image_iterator, total=len(unique_movie_infos))):
            for cluster_index in unique_movie_infos_cluster_indices[unique_movie_info_index]:
                clusters_to_render[cluster_index].left_image_url = image_urls[0]
                clusters_to_render[cluster_index].right_image_url = image_urls[1]


def _get_color_by_rating(rating):
    # From https://a.yandex-team.ru/arc/trunk/arcadia/frontend/projects/lego/packages/likes/common.blocks/rating-vendor/rating-vendor.bh.js?rev=6744173
    if rating >= 8:
        return '#32ba43'

    if rating >= 7:
        return '#89c939'

    if rating >= 5:
        return '#91a449'

    if rating >= 3:
        return '#85855d'

    return '#727272'


def _render_poster_with_rating(movie_info):
    rating_image = Image.new('RGBA', (56, 36), color=_get_color_by_rating(movie_info.rating))

    font = ImageFont.truetype('Arial_Bold.ttf', 26)
    draw = ImageDraw.Draw(rating_image)
    rating_text = '{:01.1f}'.format(movie_info.rating).replace('.', ',')
    draw.text((10, 3), rating_text, font=font, fill='white')

    rating_image = _add_corners(rating_image, radius=10)

    poster = Image.open(urllib.request.urlopen(movie_info.cover_url)).convert('RGBA')

    paste_to = (20, poster.size[1] - rating_image.size[1] - 20)
    poster.paste(rating_image, box=paste_to, mask=rating_image)

    return _upload_image(poster)


def _render_ratings(movie_infos):
    movies_to_render = [movie_info for movie_info in movie_infos if movie_info.rating > 0]
    with Pool(processes=16) as pool:
        image_iterator = pool.imap(_render_poster_with_rating, movies_to_render)

        for movie_info_index, image_url in enumerate(tqdm(image_iterator)):
            movies_to_render[movie_info_index].cover_url = image_url


def _dump_trees(movie_infos, trees, output_path):
    serialized_trees = []
    for tree in trees:
        nodes = [attr.asdict(node) for node in tree.nodes]
        for node in nodes:
            del node['movie_infos']

        serialized_trees.append({
            'filter': tree.filter_info,
            'tree_nodes': nodes,
            'content_name': tree.content_name
        })

    movie_infos = [attr.asdict(movie_info) for movie_info in movie_infos]

    result = {
        'movie_infos': movie_infos,
        'trees': serialized_trees
    }
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(result, f, ensure_ascii=False, indent=2)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--clustered-movies-path', required=True)
    args = parser.parse_args()

    logger.info('Loading data')
    movie_infos, trees = _load_data(args.clustered_movies_path)

    logger.info('Rendering clusters')
    _render_clusters(trees)

    logger.info('Rendering posters with ratings')
    _render_ratings(movie_infos)

    logger.info('Dumping info')
    _dump_trees(movie_infos, trees, args.clustered_movies_path)


if __name__ == "__main__":
    main()
