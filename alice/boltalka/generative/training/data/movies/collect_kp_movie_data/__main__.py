import argparse
import json
import os
from collections import defaultdict

from alice.boltalka.generative.training.data.movies.util import yt_read_rows, yt_write_table

KP_ROOT_FOLDER = '//home/kinopoisk/ext/production'
KP_FILMS_TABLE = os.path.join(KP_ROOT_FOLDER, 'film')
KP_ACTORS_TABLE = os.path.join(KP_ROOT_FOLDER, 'person')
KP_MOVIE_TO_PERSON_TABLE = os.path.join(KP_ROOT_FOLDER, 'data_film', 'cast')


def get_russian_title(movie_info):
    if 'titles' not in movie_info:
        return None

    for title_info in movie_info['titles']:
        if title_info.get('language', None) == 'RU':
            return title_info['attribute']
    return None


def get_actor_name(actor_info):
    for name in actor_info['names']:
        if name['language'] == 'RU':
            return name['attribute']
    return actor_info.get('originalName', None)


def get_director_id(movie_info):
    if 'participants' not in movie_info:
        return None

    for participant in movie_info['participants']:
        if participant['role'] == 'DIRECTOR':
            return int(participant['personId'])
    return None


def get_films():
    films = []

    for row in yt_read_rows(KP_FILMS_TABLE):
        key, value = row['key'], row['value']

        movie_info = json.loads(value)

        russian_title = get_russian_title(movie_info)

        genres = movie_info.get('originalGenres', [])
        if russian_title is not None:
            films.append({
                'id': int(key),
                'title': russian_title,
                'original_title': movie_info.get('originalTitle', None),
                'genre1': genres[0]['name'] if len(genres) > 0 else None,
                'genre2': genres[1]['name'] if len(genres) > 1 else None,
                'genre3': genres[2]['name'] if len(genres) > 2 else None,
                'director_id': get_director_id(movie_info)
            })
    return films


def append_actors(films, n_first_actors=3):
    movie_to_person_ids = defaultdict(list)
    for row in yt_read_rows(KP_MOVIE_TO_PERSON_TABLE):
        if row['sequence'] <= n_first_actors:
            movie_to_person_ids[int(row['film_id'])].append(int(row['person_id']))

    id_to_actor_name = dict()
    for row in yt_read_rows(KP_ACTORS_TABLE):
        key, value = row['key'], row['value']

        actor_info = json.loads(value)

        actor_name = get_actor_name(actor_info)

        if actor_name is not None:
            id_to_actor_name[int(key)] = actor_name

    for film in films:
        person_ids = movie_to_person_ids[film['id']]

        for i in range(n_first_actors):
            person_id = person_ids[i] if i < len(person_ids) else None
            film['actor_id{}'.format(i + 1)] = person_id
            film['actor_name{}'.format(i + 1)] = id_to_actor_name.get(person_id,
                                                                      None) if person_id is not None else None
        director_id = film['director_id']
        film['director_name'] = id_to_actor_name.get(director_id, None) if director_id is not None else None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--out-table', required=True, type=str
    )
    args = parser.parse_args()

    films = get_films()
    append_actors(films)

    # Debug output for sanity
    for film in films[-10:-1]:
        print('FILM!')
        for k, v in film.items():
            print('{} -> {}'.format(k, v))
        print('========================')

    yt_write_table(films, args.out_table)


if __name__ == '__main__':
    main()
