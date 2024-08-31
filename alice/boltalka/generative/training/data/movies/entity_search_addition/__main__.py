import argparse

import requests

import yt.wrapper as yt
from alice.boltalka.generative.training.data.movies.util import yt_read_rows

ENTITYSEARCH_QUERY = 'http://entitysearch.yandex.net/get?obj=kin0{}'


def try_set_entity_search(row):
    kp_id = row['id']
    query_path = ENTITYSEARCH_QUERY.format(kp_id)
    response = requests.get(query_path)
    if not response.ok:
        raise ValueError('{} returned code {}'.format(query_path, response.status_code))
    row['entity_search'] = response.json()


def append_to_table(path, rows):
    print('Appending {} rows to {}'.format(len(rows), path))
    yt.write_table(yt.TablePath(path, append=True), rows)


def set_entity_search(table_path_in, table_path_out, batch_size=10000):
    tmp_table = yt.create_temp_table()
    current_batch = []
    for row in yt_read_rows(table_path_in):
        try_set_entity_search(row)

        current_batch.append(row)

        if len(current_batch) == batch_size:
            append_to_table(tmp_table, current_batch)
            current_batch = []

    if len(current_batch) > 0:
        append_to_table(tmp_table, current_batch)

    yt.move(tmp_table, table_path_out, force=True)
    print('Moved {} to {}'.format(tmp_table, table_path_out))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--kinopoisk-data-in',
        default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/kp_movie_data', type=str
    )
    parser.add_argument(
        '--kinopoisk-data-out',
        default='//home/voice/artemkorenev/boltalka/ugc/3rd_party/kp_movie_data_with_entity_search', type=str
    )
    args = parser.parse_args()

    set_entity_search(args.kinopoisk_data_in, args.kinopoisk_data_out)


if __name__ == '__main__':
    main()
