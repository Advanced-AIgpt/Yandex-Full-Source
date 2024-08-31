# coding: utf-8
from __future__ import unicode_literals

import logging
import json
import codecs
import os
import argparse

from collections import defaultdict
from numpy import random as random

from music_database import MusicDatabase
from vins_core.utils.lemmer import Inflector
from vins_core.utils.data import get_resource_full_path, find_file, find_dir

_inflector = Inflector('ru')

logger = logging.getLogger(__name__)

QUERY_TARGET_TYPE = ['abstract', 'specific']
QUERY_TARGET_PROBABILITIES = [0.5, 0.5]
ABSTRACT_QUERY_TARGET_TYPE = ['more/less', 'other']
ABSTRACT_QUERY_TARGET_PROBABILITIES = [0.3, 0.7]
SPECIFIC_QUERY_TARGET_TYPE = ['playlist', 'special_playlist', 'artist', 'album', 'track']
SPECIFIC_QUERY_TARGET_PROBABILITIES = [0.1, 0.15, 0.25, 0.1, 0.4]
COIN = [True, False]
BIASED_COIN_PROBABILITIES = [0.2, 0.8]


class MusicQuerySampler:
    def __init__(self, music_database, dangerous_words):
        self._music_database = music_database
        self._dangerous_words = dangerous_words

    @staticmethod
    def sample_genre():
        return random.choice([
            "rusfolk",
            "lounge",
            "house",
            "pop",
            "tradjazz",
            "reggaeton",
            "rusrap",
            "rusestrada",
            "rusrock",
            "hardcore",
            "rusbards",
            "films",
            "vocal",
            "dubstep",
            "urban",
            "local-indie",
            "industrial",
            "classicmetal",
            "tatar",
            "newage",
            "conjazz",
            "romances",
            "techno",
            "tvseries",
            "modern",
            "dub",
            "soul",
            "indie",
            "trance",
            "meditation",
            "caucasian",
            "rnr",
            "progmetal",
            "animated",
            "experimental",
            "funk",
            "ruspop",
            "numetal",
            "celtic",
            "rock",
            "videogame",
            "prog",
            "dnb",
            "musical",
            "epicmetal",
            "disco",
            "metal",
            "postrock",
            "alternative",
            "bollywood",
            "newwave",
            "balkan",
            "folkmetal",
            "electronics",
            "extrememetal",
            "dance",
            "eurofolk",
            "rap",
            "jewish",
            "rnb",
            "jazz",
            "blues",
            "reggae",
            "ska",
            "eastern",
            "punk",
            "folk",
            "african",
            "classical",
            "latinfolk",
            "estrada",
            "amerfolk",
            "shanson",
            "country",
            "soundtrack",
            # "relax",
            "bard",
            "forchildren"
        ])

    @staticmethod
    def sample_mood():
        return random.choice([
            "aggressive",
            "spring",
            "sad",
            "rainy",
            "healing",
            "winter",
            "beautiful",
            "cool",
            "summer",
            "dream",
            "haunting",
            "dark",
            "newyear",
            "autumn",
            "happy",
            "relaxed",
            "sentimental",
            # "calm",
            "energetic",
            "epic"
        ])

    @staticmethod
    def sample_language():
        return random.choice([
            "russian",
            "russian",
            "russian",
            "russian",
            "russian",
            "not-russian",
            "not-russian",
            "not-russian",
            "not-russian",
            "not-russian",
            "spanish",
            "french",
            "german",
            "italian",
            "portuguese",
            "swedish",
            "netherlandish",
            "ukrainian",
            "romanian",
            "swahili",
            "turkish",
            "finnish",
            "bosnian",
            "malay"
        ])

    @staticmethod
    def sample_activity():
        return random.choice([
            "wake-up",
            "work-background",
            "workout",
            "driving",
            "road-trip",
            "party",
            "beloved",
            "sex",
            "fall-asleep"
        ])

    @staticmethod
    def sample_epoch():
        return random.choice([
            "the-greatest-hits",
            "fifties",
            "sixties",
            "seventies",
            "eighties",
            "nineties",
            "zeroes"
        ])

    @staticmethod
    def sample_vocal():
        return random.choice([
            "male",
            "female",
            "instrumental"
        ])

    def sample_playlist_query(self, context, choosing):
        if 'playlist_or_album' in choosing:
            return None

        choosing.add('playlist_or_album')

        return {
            'type': 'playlist',
            'name': context.sample_label()
        }

    def sample_special_playlist_query(self):
        return {
            'type': 'special_playlist',
            'id': random.choice([
                'alice', 'chart', 'ny_alice_playlist', 'playlist_of_the_day',
                'recent_tracks', 'missed_likes', 'never_heard'
            ])
        }

    def sample_artist_query(self, context, choosing, similarity_ok=False):
        if 'artist' in choosing:
            return None

        choosing.add('artist')
        label = context.sample_label()

        return {
            'type': 'artist',
            'name': label,
            'definite': 'true' if label not in self._dangerous_words else 'false',
            'need_similar': 'need_similar' if similarity_ok and (
                random.choice(COIN, p=BIASED_COIN_PROBABILITIES)
            ) else 'exact'
        }

    def sample_album_query(self, context, named, choosing):
        if 'playlist_or_album' in choosing:
            return None

        choosing.add('playlist_or_album')

        res = {
            'type': 'album',
        }

        if named:
            label = context.sample_label()
            res['name'] = label
            res['definite'] = 'true' if label not in self._dangerous_words else 'false'
        else:
            if not context.artists:
                return None

        if (not named or random.choice(COIN)) and context.artists:
            artist_query = self.sample_artist_query(random.choice(context.artists), choosing)
            if artist_query:
                res['artist'] = artist_query

            if not named and random.choice(COIN, p=BIASED_COIN_PROBABILITIES):
                res['novelty'] = 'new'

        return res

    def sample_track_query(self, context, named, choosing, music_database, more_less_mode=False, similarity_ok=False):
        res = {
            'type': 'track'
        }

        if named:
            label = context.sample_label()
            res['name'] = label
            res['definite'] = 'true' if label not in self._dangerous_words else 'false'

        for _ in range(1 if named else 2):
            if (_ or not more_less_mode) and random.choice(COIN):
                continue

            if named:
                choices = ['artist', 'album']
            elif more_less_mode:
                choices = [
                    'genre', 'mood', 'mood', 'activity', 'epoch', 'language', 'vocal', 'novelty', 'personality'
                ]
                if context.artists:
                    choices.append('artist')
            else:
                choices = [
                    'genre', 'mood', 'activity', 'epoch', 'artist', 'playlist',
                    'album', 'language', 'vocal', 'novelty', 'personality'
                ]

            parameter = random.choice([l for l in choices if l not in res])

            if parameter == 'genre':
                res['genre'] = self.sample_genre()
            elif parameter == 'mood':
                res['mood'] = self.sample_mood()
            elif parameter == 'language':
                res['language'] = self.sample_language()
            elif parameter == 'vocal':
                res['vocal'] = self.sample_vocal()
            elif parameter == 'epoch':
                res['epoch'] = self.sample_epoch()
            elif parameter == 'activity':
                res['activity'] = self.sample_activity()
            elif parameter == 'novelty':
                res['novelty'] = 'new'
            elif parameter == 'personality':
                res['personality'] = 'is_personal'
            elif parameter == 'artist' and context.artists:
                artist_query = self.sample_artist_query(random.choice(context.artists), choosing)
                if artist_query:
                    res['artist'] = artist_query
            elif parameter == 'playlist':
                playlist_query = self.sample_playlist_query(
                    music_database.sample_random_playlist(), choosing
                )
                if playlist_query:
                    res['playlist'] = playlist_query
            elif parameter == 'album' and context.albums:
                album_query = self.sample_album_query(random.choice(context.albums), True, choosing)
                if album_query:
                    res['album'] = album_query

        res['need_similar'] = 'need_similar' if similarity_ok and (
            random.choice(COIN, p=BIASED_COIN_PROBABILITIES)
        ) else 'exact'

        return res

    @staticmethod
    def _find_names(data):
        if isinstance(data, list):
            for e in data:
                for name in MusicQuerySampler._find_names(e):
                    yield name
        elif isinstance(data, dict):
            for k, v in data.iteritems():
                if k == 'name':
                    yield v
                else:
                    for name in MusicQuerySampler._find_names(v):
                        yield name

    @staticmethod
    def generate_inflection_info(queries):
        result = {}

        for name in set(MusicQuerySampler._find_names(queries)):
            try:
                acc_form = _inflector.inflect(name, {'acc'})
            except RuntimeError:
                logger.warning('Error during inflection of %s' % name)
                acc_form = name

            try:
                gen_form = _inflector.inflect(name, {'gen'})
            except RuntimeError:
                logger.warning('Error during inflection of %s' % name)
                gen_form = name

            result[name] = [
                [acc_form, ['acc', 'noun']],
                [gen_form, ['gen', 'noun']]
            ]

        return result

    def sample_query(self):
        target_type = random.choice(QUERY_TARGET_TYPE, p=QUERY_TARGET_PROBABILITIES)

        predicate = 'switch_on'
        shuffle_is_ok = True

        if target_type == 'specific':
            specific_target_type = random.choice(SPECIFIC_QUERY_TARGET_TYPE, p=SPECIFIC_QUERY_TARGET_PROBABILITIES)

            if specific_target_type == 'playlist':
                result = self.sample_playlist_query(self._music_database.sample_random_playlist(), set())
            elif specific_target_type == 'special_playlist':
                result = self.sample_special_playlist_query()
            elif specific_target_type == 'artist':
                if random.choice(COIN, p=BIASED_COIN_PROBABILITIES):
                    predicate = random.choice(['more', 'less'])
                    similarity_ok = False
                else:
                    similarity_ok = True
                result = self.sample_artist_query(self._music_database.sample_random_artist(), set(), similarity_ok)
            elif specific_target_type == 'album':
                result = self.sample_album_query(self._music_database.sample_random_album(), True, set())
            elif specific_target_type == 'track':
                shuffle_is_ok = False
                result = self.sample_track_query(
                    self._music_database.sample_random_track(), True, set(), music_database, False, True
                )
            else:
                assert False
        else:
            specific_target_type = random.choice(ABSTRACT_QUERY_TARGET_TYPE, p=ABSTRACT_QUERY_TARGET_PROBABILITIES)

            if specific_target_type == 'more/less':
                predicate = random.choice(['more', 'less'])

            if random.choice(COIN, p=BIASED_COIN_PROBABILITIES):
                album_query = self.sample_album_query(
                    self._music_database.sample_random_album(), False, set()
                )

                if not album_query:
                    return None

                result = {
                    'type': 'collection',
                    'proto': album_query
                }
            else:
                result = {
                    'type': 'collection',
                    'proto': self.sample_track_query(
                        self._music_database.sample_random_track(), False, set(), music_database,
                        specific_target_type == 'more/less'
                    )
                }

        result = {
            'type': predicate,
            'target': result
        }

        if predicate == 'switch_on':
            if shuffle_is_ok and random.choice(COIN, p=BIASED_COIN_PROBABILITIES):
                result['order'] = 'shuffle'
            if random.choice(COIN, p=BIASED_COIN_PROBABILITIES):
                result['repeat'] = 'repeat'

        return result


def sample_queries(music_database, dangerous_words, count, target_path, target_inflection_info_path):
    query_sampler = MusicQuerySampler(music_database, dangerous_words)
    result = []

    count_more_less_plus_mood = 0

    for i in range(count):
        if not (i % 100):
            print(i)

        query = None
        while not query:
            query = query_sampler.sample_query()

        if query['type'] in ('more', 'less') and 'proto' in query['target']:
            assert len(query['target']['proto']) > 1
            if len(query['target']['proto']) == 3 and 'mood' in query['target']['proto']:
                count_more_less_plus_mood += 1

        result.append(query)

    with codecs.open(target_path, mode='w', encoding='utf-8') as fout:
        json.dump(result, fout, indent=2, ensure_ascii=False)

    with codecs.open(target_inflection_info_path, mode='w', encoding='utf-8') as fout:
        json.dump(query_sampler.generate_inflection_info(result), fout, indent=2, ensure_ascii=False)

    logger.info('Found %d more/less + mood queries' % count_more_less_plus_mood)


def _count_label_statistics(all_object_iterators):
    stat = defaultdict(dict)

    for object_type, object_iter in all_object_iterators.iteritems():
        for obj, obj_prob in object_iter:
            for label, label_prob in obj.yield_labels(threshold=0.05):
                label_stat = stat[label]

                freq = float(obj.frequency) * label_prob
                label_stat[object_type] = label_stat.get(object_type, 0.0) + freq

    return sorted(stat.items(), key=lambda x: -sum(x[1].values()))


def build_custom_entities(music_database, dangerous_words, stats_path, out_path, build_playlists=False):
    all_object_iterators = {
        'track': music_database.yield_tracks(300000),
        'album': music_database.yield_albums(100000),
        'artist': music_database.yield_artists(200000)
    }

    if build_playlists:
        all_object_iterators['playlist'] = music_database.yield_playlists(200000)

    label_stats = _count_label_statistics(all_object_iterators)

    target = {
        'artist': [],
        'album': [],
        'track': []
    }

    if build_playlists:
        target['playlist'] = []

    with codecs.open(stats_path, mode='w', encoding='utf-8') as fout:
        json.dump(label_stats, fout, indent=2, ensure_ascii=False)

    for label, label_stat in label_stats:
        if label in dangerous_words:
            continue

        types = target.keys()
        frequencies = [label_stat.get(tp) or 0.0 for tp in types]
        sum_freq = sum(frequencies)
        probs = [freq / sum_freq for freq in frequencies]

        for tp, prob in zip(types, probs):
            if prob < 0.001:
                continue

            if prob > 0.999:
                target[tp].append(label)
            else:
                target[tp].append([label, prob])

    for tp, labels in target.items():
        logger.info("Dumping %d %s labels" % (len(labels), tp))
        full_path = os.path.join(out_path, tp + '.json')

        with codecs.open(full_path, mode='w', encoding='utf-8') as fout:
            json.dump({tp: labels}, fout, indent=2, ensure_ascii=False)


def load_dangerous_words(custom_entities_dirs_or_files):
    files_to_load = []
    result = set()

    for path in custom_entities_dirs_or_files:
        if os.path.isdir(path):
            for fname in os.listdir(path):
                name = os.path.basename(fname)
                file_path = os.path.join(path, name)

                name, ext = os.path.splitext(name)

                if os.path.isfile(file_path) and ext == '.json':
                    files_to_load.append(file_path)
        elif path.endswith('.json'):
            files_to_load.append(path)

    for file in files_to_load:
        with codecs.open(file, 'r', encoding='utf-8') as f:
            source = json.load(f)
            if isinstance(source, dict):
                for _, synonims in source.items():
                    for s in synonims:
                        result.add(s.lower().strip())
            else:
                for s in source:
                    result.add(s.lower().strip())

    return result


def filter_non_editorial(playlists_iterator):
    for playlist, prob in playlists_iterator:
        if not playlist.is_editorial:
            continue

        yield playlist, prob


def collect_editorial_playlist_labels(music_database, dangerous_words):
    all_object_iterators = {
        'playlist': filter_non_editorial(music_database.yield_playlists())
    }

    return [
        [label, {'frequency': label_stats['playlist'], 'is_dangerous': any([w in label for w in dangerous_words])}]
        for label, label_stats in _count_label_statistics(all_object_iterators)
    ]


if __name__ == "__main__":
    FORMAT = '%(asctime)-15s %(message)s'
    logging.basicConfig(format=FORMAT, level=logging.INFO)

    parser = argparse.ArgumentParser(description='Generate music queries that can be used to create synthetic nlu.\n'
                                                 'Also generates data that is needed to construct \n'
                                                 'music fst-s (albums,tracks,artists).\n'
                                                 'Usage: python music_query_sampler.py '
                                                 '--config-path=<path to configuration file> ',
                                     formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('--config-path', type=str, required=True, help='path to generator json config')
    args = parser.parse_args()
    config_file = open(args.config_path)
    config = json.load(config_file)
    config_dir = os.path.dirname(args.config_path)

    music_database = MusicDatabase(
        open(get_resource_full_path(config['music_database_dump_sandbox_resource_id']), 'r')
    )

    print("Loaded links stat:\n\t%s" % '\n\t'.join(
        [str(e) for e in music_database.get_loaded_links_stat()]
    ))
    print("Skipped links stat:\n\t%s" % '\n\t'.join(
        [str(e) for e in music_database.get_skipped_links_stat()]
    ))

    dangerous_words = load_dangerous_words(
        [
            find_file('personal_assistant', path) if path.endswith('.json') else find_dir('personal_assistant', path)
            for path in config['dangerous_words_sources']
        ]
    )

    editorial_playlist_labels = collect_editorial_playlist_labels(music_database, dangerous_words)
    logger.info("Dumping %d editorial playlist labels" % len(editorial_playlist_labels))
    editorial_playlists_path = os.path.join(config_dir, config['editorial_playlists_out_file'])

    with codecs.open(editorial_playlists_path, mode='w', encoding='utf-8') as fout:
        json.dump(editorial_playlist_labels, fout, indent=2, ensure_ascii=False)

    custom_entities_output_dir = os.path.join(config_dir, config['custom_entities_output_dir'])
    if not os.path.exists(custom_entities_output_dir):
        os.makedirs(custom_entities_output_dir)

    build_custom_entities(
        music_database,
        dangerous_words,
        os.path.join(config_dir, config['label_stats_out_file']),
        custom_entities_output_dir
    )

    sample_queries(
        music_database, dangerous_words, config['music_queries_count'],
        os.path.join(config_dir, config['music_queries_out_file']),
        os.path.join(config_dir, config['music_queries_inflection_info_out_file'])
    )
