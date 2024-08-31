# -*- coding: utf-8 -*-

import argparse
import attr
import codecs
import json
import logging
import requests
import yt.wrapper as yt

from tqdm import tqdm
from multiprocessing import Pool

from concurrent.futures import TimeoutError
import ydb

from alice.hollywood.library.scenarios.suggesters.movie_akinator.content_preparer.common import (
    SimilarMovieInfo, MovieInfo
)

logger = logging.getLogger(__name__)


_GENRE_MAPPING = {
    u'анимационный': u'childrens',
    u'аниме': u'anime',
    u'биография': u'biopic',
    u'боевик': u'action',
    u'вестерн': u'westerns',
    u'военный': u'war',
    u'детектив': u'detective',
    u'детский': u'childrens',
    u'документальный': u'documentary',
    u'драма': u'drama',
    u'история': u'historical',
    u'комедия': u'comedy',
    u'концерт': u'concert',
    u'криминал': u'crime',
    u'мелодрама': u'melodramas',
    u'музыка': u'musical',
    u'мультфильм': u'childrens',
    u'мюзикл': u'musical',
    u'приключения': u'adventure',
    u'семейный': u'family',
    u'спорт': u'sport_video',
    u'триллер': u'thriller',
    u'ужасы': u'horror',
    u'фантастика': u'science_fiction',
    u'фильм-нуар': u'noir',
    u'фэнтези': u'fantasy'
}


def _get_head(session, db_path):
    head_query = '''
        PRAGMA TablePathPrefix = '/ru/alice/prod/bass/{}';
        SELECT * FROM video_items_v5
        ORDER BY KinopoiskId;
    '''

    return session.transaction(ydb.SerializableReadWrite()).execute(
        head_query.format(db_path),
        commit_tx=True,
    )[0].rows


def _get_tail(session, db_path, last_kinopoisk_id):
    tail_query = '''
        PRAGMA TablePathPrefix = '/ru/alice/prod/bass/{}';
        SELECT * FROM video_items_v5
        WHERE KinopoiskId > '{}'
        ORDER BY KinopoiskId;
    '''

    return session.transaction(ydb.SerializableReadWrite()).execute(
        tail_query.format(db_path, last_kinopoisk_id),
        commit_tx=True,
    )[0].rows


def _iterate_rows(session, db_path):
    result_sets = _get_head(session, db_path)

    while True:
        if not result_sets:
            return
        yield result_sets

        last_kinopoisk_id = result_sets[-1].KinopoiskId
        result_sets = _get_tail(session, db_path, last_kinopoisk_id)


def _download_bass_data(db_path, auth_token):
    connection_params = ydb.DriverConfig(
        'ydb-ru.yandex.net:2135',
        database='/ru/alice/prod/bass',
        auth_token=auth_token
    )

    with ydb.Driver(connection_params) as driver:
        try:
            driver.wait(timeout=5)
        except TimeoutError:
            raise RuntimeError('Connect failed to YDB')

        session = driver.table_client.session().create()

        result = []
        for rows in _iterate_rows(session, db_path):
            result.extend(rows)

    return result


def _load_similarities(similarities_table_path):
    logger.info('Loading similarities from %s', similarities_table_path)

    onto_id_to_similar_movies = {}
    for row in yt.read_table(similarities_table_path):
        onto_id_to_similar_movies[row['id']] = [
            SimilarMovieInfo(onto_id, similarity) for onto_id, similarity in row['similarities']
        ]

    logger.info('Loaded %s samples', len(onto_id_to_similar_movies))
    logger.info('Example: %s', next(onto_id_to_similar_movies.iteritems()))
    return onto_id_to_similar_movies


def _load_popularities(popularities_table_path):
    logger.info('Loading popularities from %s', popularities_table_path)

    kp_id_to_watch_count = {}
    for row in yt.read_table(popularities_table_path):
        kp_id_to_watch_count[int(row['film_id'])] = row['watch_count']

    logger.info('Loaded %s samples', len(kp_id_to_watch_count))
    logger.info('Example: %s', next(kp_id_to_watch_count.iteritems()))
    return kp_id_to_watch_count


def _get_content_type(item):
    if u'мультфильм' in item.get('genre', ''):
        return 'cartoon'
    return item['type']


def _parse_movie_info(raw_movie_info):
    video_item = json.loads(raw_movie_info.Content)

    kinopoisk_genres = video_item.get('genre', '')
    genres = [_GENRE_MAPPING[genre] for genre in kinopoisk_genres.split(', ') if genre in _GENRE_MAPPING]

    movie_info = MovieInfo(
        name=video_item['name'],
        kinopoisk_id=int(raw_movie_info.KinopoiskId),
        kinopoisk_url=video_item['debug_info']['web_page_url'],
        cover_url=video_item['cover_url_2x3'],
        min_age=video_item.get('min_age', 0),
        content_type=_get_content_type(video_item),
        genres=genres,
        description=video_item['description'].strip(),
        rating=video_item.get('rating', 0.)
    )
    assert int(movie_info.kinopoisk_url.split('/')[-1]) == movie_info.kinopoisk_id

    return movie_info


def _get_kinopoisk_id(onto_id):
    try:
        if onto_id.startswith('kin'):
            return ('film/{}'.format(int(onto_id[3:])), onto_id)
        response = requests.get('http://entitysearch.yandex.net/get?obj={}'.format(onto_id))
        return (response.json()['cards'][0]['base_info']['ids']['kinopoisk'], onto_id)
    except Exception as e:
        logger.info('Could not get kinopoisk_id, an error occurred: %s', e)
        return (None, onto_id)


def _map_kinopoisk_to_onto_ids(onto_ids):
    logger.info('Collecting kp to onto id mapping')

    kp_to_onto_id = {}
    pool = Pool(processes=64)

    for kp_id, onto_id in tqdm(pool.imap_unordered(_get_kinopoisk_id, onto_ids), total=len(onto_ids)):
        if kp_id:
            kp_to_onto_id[int(kp_id[5:])] = onto_id

    pool.close()
    pool.join()

    logger.info('Example: %s', next(kp_to_onto_id.iteritems()))
    return kp_to_onto_id


def _collect_movie_infos(bass_db_path, ydb_auth_token, kp_id_to_popularity, kp_to_onto_id, onto_id_to_similar_movies):
    logger.info('Loading movie infos')

    movie_infos = []
    for raw_movie_info in _download_bass_data(bass_db_path, ydb_auth_token):
        movie_info = _parse_movie_info(raw_movie_info)

        popularity = kp_id_to_popularity.get(movie_info.kinopoisk_id)
        if not popularity:
            logger.info('Popularity for the kinopoisk id %s was not found', movie_info.kinopoisk_id)
            continue

        onto_id = kp_to_onto_id.get(movie_info.kinopoisk_id)
        if not onto_id:
            logger.info('Onto id for the kinopoisk id %s was not found', movie_info.kinopoisk_id)
            continue

        movie_info.popularity = popularity
        movie_info.onto_id = onto_id
        movie_info.similar_movie_infos = onto_id_to_similar_movies[onto_id]

        movie_infos.append(movie_info)

    return movie_infos


def _collect_data(bass_db_path, ydb_auth_token, similarities_table_path, popularities_table_path):
    onto_id_to_similar_movies = _load_similarities(similarities_table_path)
    kp_to_onto_id = _map_kinopoisk_to_onto_ids(onto_id_to_similar_movies.keys())

    kp_id_to_popularity = _load_popularities(popularities_table_path)

    data = _collect_movie_infos(bass_db_path, ydb_auth_token, kp_id_to_popularity,
                                kp_to_onto_id, onto_id_to_similar_movies)

    logging.info('Collected %s movie infos', len(data))

    return data


def _dump_data(data, output_path):
    with codecs.open(output_path, 'w', encoding='utf-8') as f:
        data = [attr.asdict(item) for item in data]
        json.dump(data, f, ensure_ascii=False, encoding='utf-8', indent=2)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--video-items-path', required=True,
        help='Path to video_items_v5 table (from ru/alice/prod/bass/browser/<>/video_items_v5)'
    )
    parser.add_argument('--auth-token', required=True, help='YDB OAuth token')
    parser.add_argument('--output-path', required=True, help='Path to result json')

    parser.add_argument('--similarities-table-path',
                        default='//home/voice/dan-anastasev/movie_akinator/movie_slim_similarities')
    parser.add_argument('--popularities-table-path',
                        default='//home/voice/dan-anastasev/movie_akinator/movie_popularity')

    args = parser.parse_args()

    data = _collect_data(args.video_items_path, args.auth_token,
                         args.similarities_table_path, args.popularities_table_path)
    _dump_data(data, args.output_path)


if __name__ == "__main__":
    main()
