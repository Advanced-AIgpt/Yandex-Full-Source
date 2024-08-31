# -*- coding: utf-8 -*-

import argparse
import attr
import codecs
import json
import logging
import requests

from concurrent.futures import TimeoutError
import ydb

logger = logging.getLogger(__name__)


@attr.s
class Suggestion(object):
    text = attr.ib()
    kinopoisk_id = attr.ib()
    persuading_text = attr.ib(default=None)
    video_item = attr.ib(default=None)
    name = attr.ib(default=None)
    item_id = attr.ib(default=None)
    genre = attr.ib(default=None)
    min_age = attr.ib(default=None)
    content_type = attr.ib(default=None)
    bass_genres = attr.ib(default=None)


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


def _normalize_text(text):
    params = {
        'format': 'json', 'wizclient': 'megamind',
        'rwr': ['AliceNormalizer'], 'text': text
    }

    response = requests.get('http://hamzard.yandex.net:8891/wizard', params)
    return response.json()['rules']['AliceNormalizer']['NormalizedRequest']


def _get_head(session, db_path, kinopoisk_ids):
    head_query = '''
        PRAGMA TablePathPrefix = "/ru/alice/prod/bass/{}";
        SELECT * FROM video_items_v5
        WHERE KinopoiskId IN ({})
        ORDER BY KinopoiskId;
    '''

    return session.transaction(ydb.SerializableReadWrite()).execute(
        head_query.format(db_path, kinopoisk_ids),
        commit_tx=True,
    )[0].rows


def _get_tail(session, db_path, kinopoisk_ids, last_kinopoisk_id):
    tail_query = '''
        PRAGMA TablePathPrefix = "/ru/alice/prod/bass/{}";
        SELECT * FROM video_items_v5
        WHERE KinopoiskId IN ({}) AND KinopoiskId > "{}"
        ORDER BY KinopoiskId;
    '''

    return session.transaction(ydb.SerializableReadWrite()).execute(
        tail_query.format(db_path, kinopoisk_ids, last_kinopoisk_id),
        commit_tx=True,
    )[0].rows


def _iterate_rows(session, db_path, kinopoisk_ids):
    result_sets = _get_head(session, db_path, kinopoisk_ids)

    while True:
        if not result_sets:
            return
        yield result_sets

        last_kinopoisk_id = result_sets[-1].KinopoiskId
        result_sets = _get_tail(session, db_path, kinopoisk_ids, last_kinopoisk_id)


def _download_data(db_path, auth_token, kinopoisk_ids):
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
        kinopoisk_ids = ', '.join('"{}"'.format(kinopoisk_id) for kinopoisk_id in kinopoisk_ids)

        result = []
        for rows in _iterate_rows(session, db_path, kinopoisk_ids):
            result.extend(rows)

    return result


def _preprocess_video_item(video_item):
    video_item = json.loads(video_item)

    video_item['available'] = 1

    provider_info = video_item['provider_info'][0]
    assert provider_info['provider_name'] == 'kinopoisk'
    video_item['availability_request'] = {
        provider_info['provider_name']: {
            'id': provider_info['provider_item_id'],
        },
        'type': 'film'
    }

    video_item['source'] = 'video_source_kp_recommendations'
    video_item['normalized_name'] = _normalize_text(video_item['name'])

    return video_item


def _get_content_type(item):
    if u'мультфильм' in item.get('genre', ''):
        return 'cartoon'
    return item['type']


def _update_suggestions(suggestions, db_path, auth_token):
    suggestions = {suggestion.kinopoisk_id: suggestion for suggestion in suggestions}

    downloaded_rows = _download_data(db_path, auth_token, suggestions.keys())

    skipped_ids = ', '.join(set(suggestions.keys()) - {row.KinopoiskId for row in downloaded_rows})
    if skipped_ids:
        logger.warning('Did not download data for following kinopoisk ids: %s', skipped_ids)

    result = []
    for row in downloaded_rows:
        suggestion = suggestions[row.KinopoiskId]
        suggestion.video_item = _preprocess_video_item(row.Content)
        suggestion.name = suggestion.video_item['name']
        suggestion.item_id = row.ProviderItemId
        suggestion.genre = suggestion.video_item['genre']
        suggestion.min_age = suggestion.video_item.get('min_age', 0)
        suggestion.content_type = _get_content_type(suggestion.video_item)
        suggestion.bass_genres = [
            _GENRE_MAPPING[genre] for genre in suggestion.video_item['genre'].split(', ') if genre in _GENRE_MAPPING
        ]
        result.append(suggestion)

    return result


def _load_inputs(data_path):
    suggestions = []
    with open(data_path) as f:
        header = f.readline()
        assert header.strip() == 'kinopoisk_id\tsuggestion_text'

        for line in f:
            kinopoisk_id, texts = line.strip().split('\t')
            texts = json.loads(texts)
            text = texts[0]
            if len(texts) == 2:
                persuading_text = texts[1]
            else:
                persuading_text = None
                assert len(texts) == 1

            suggestions.append(Suggestion(text=text, kinopoisk_id=kinopoisk_id, persuading_text=persuading_text))

    return suggestions


def _dump_results(output_data_path, suggestions):
    suggestions = [
        attr.asdict(suggestion) for suggestion in suggestions if suggestion.content_type in {'cartoon', 'movie'}
    ]

    with codecs.open(output_data_path, 'w', encoding='utf8') as f:
        json.dump(suggestions, f, indent=2, ensure_ascii=False, encoding='utf8')


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--video-items-path', required=True,
        help='Path to video_items_v5 table (from ru/alice/prod/bass/browser/<>/video_items_v5)'
    )
    parser.add_argument('--auth-token', required=True, help='YDB OAuth token')
    parser.add_argument(
        '--input-data', required=True, help='Path to tsv file with kinopoisk_id and suggestion_text columns'
    )
    parser.add_argument('--output-data', required=True, help='Path to result json')

    args = parser.parse_args()

    suggestions = _load_inputs(args.input_data)
    _update_suggestions(suggestions, args.video_items_path, args.auth_token)
    _dump_results(args.output_data, suggestions)


if __name__ == "__main__":
    main()
