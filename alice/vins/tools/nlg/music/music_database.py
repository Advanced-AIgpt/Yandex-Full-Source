# coding: utf-8
from __future__ import unicode_literals

import logging
import json
import attr
import codecs
import numpy
import re

from itertools import izip
from vins_core.ner.fst_normalizer import NluFstNormalizer

logger = logging.getLogger(__name__)


@attr.s
class MusicObject(object):
    identifier = attr.ib()
    labels = attr.ib(default=attr.Factory(list))
    label_probs = attr.ib(default=attr.Factory(list))
    frequency = attr.ib(default=None)

    def sample_label(self):
        return numpy.random.choice(self.labels, p=self.label_probs)

    def yield_labels(self, threshold=0.0):
        for label, prob in izip(self.labels, self.label_probs):
            if prob > threshold:
                yield label, prob


@attr.s
class Playlist(MusicObject):
    is_editorial = attr.ib(default=False)


@attr.s
class Artist(MusicObject):
    pass


@attr.s
class Album(MusicObject):
    artists = attr.ib(default=None)


@attr.s
class Track(MusicObject):
    artists = attr.ib(default=None)
    albums = attr.ib(default=None)
    playlists = attr.ib(default=None)


class MusicDatabase(object):
    def _read_music_object_list(self, music_objects, object_class, external_object):
        result = []
        class_name = object_class.__name__.lower()
        for object_id in music_objects:
            music_object = self._music_objects.get((class_name, str(object_id)))
            if not music_object:
                logger.debug(
                    'Unknown %s id %s within object %s' % (
                        class_name, object_id, external_object.identifier
                    )
                )
                self._skipped_links_stats[class_name] += 1
                continue
            if not isinstance(music_object, object_class):
                logger.debug(
                    'Object %s expected to be %s because it is linked as %s within object %s, but it is %s' %
                    (
                        music_object.identifier, class_name, class_name,
                        external_object.identifier, music_object.__class__.__name__.lower()
                    )
                )
                self._skipped_links_stats[class_name] += 1
                continue

            self._loaded_links_stats[class_name] += 1
            result.append(music_object)

        return result

    @staticmethod
    def _prepare_single_label(label, is_playlist):
        label = label.strip().lower()

        if is_playlist:
            label = re.sub(
                r'^(([^:]+:|плейлист|плэйлист|подборка|треклист|трэклист|сборник) +)?(.*)',
                '\\3', label, flags=re.UNICODE
            )

        splitted_label = label.split()

        if len(splitted_label) > 10:
            label = ' '.join(splitted_label[:10])

        return label

    @staticmethod
    def _prepare_labels(label, aliases, ruglish_aliases, anchors, music_object):
        labels = []
        label_probs = []
        all_labels = set()
        sum_freq = 0.0

        is_playlist = isinstance(music_object, Playlist)

        prepared_label = MusicDatabase._prepare_single_label(label, is_playlist)
        if not NluFstNormalizer.has_unknown_symbol(prepared_label):
            labels.append(prepared_label)
            label_probs.append(music_object.frequency)
            sum_freq += float(music_object.frequency)
            all_labels.add(prepared_label)

        if is_playlist:
            for alias in aliases:
                prepared_label = MusicDatabase._prepare_single_label(alias, is_playlist)
                if prepared_label in all_labels:
                    continue

                if NluFstNormalizer.has_unknown_symbol(prepared_label):
                    continue

                labels.append(prepared_label)
                label_probs.append(music_object.frequency)
                sum_freq += float(music_object.frequency)
                all_labels.add(prepared_label)

        for ruglish_alias in ruglish_aliases:
            prepared_label = MusicDatabase._prepare_single_label(ruglish_alias, is_playlist)
            if prepared_label in all_labels:
                continue

            if NluFstNormalizer.has_unknown_symbol(prepared_label):
                continue

            labels.append(prepared_label)
            label_probs.append(music_object.frequency * 1.3)
            sum_freq += float(music_object.frequency * 1.3)
            all_labels.add(prepared_label)

        for anchor, freq in anchors.items():
            prepared_label = MusicDatabase._prepare_single_label(anchor, is_playlist)
            if prepared_label in all_labels:
                continue

            if not any(prepared_label in lbl for lbl in labels):
                continue

            if isinstance(music_object, Album) and music_object.artists:
                found = False
                for art in music_object.artists:
                    for lbl in art.labels:
                        if lbl == prepared_label:
                            found = True
                            break
                if found:
                    continue

            if NluFstNormalizer.has_unknown_symbol(prepared_label):
                continue

            labels.append(prepared_label)
            label_probs.append(freq)
            sum_freq += float(freq)
            all_labels.add(prepared_label)

        return labels, [freq / sum_freq for freq in label_probs]

    def __init__(self, input):
        self._music_objects = {}
        self._playlists = []
        self._playlists_probs = []
        self._playlists_choices_cache = None
        self._artists = []
        self._artists_probs = []
        self._artists_choices_cache = None
        self._albums = []
        self._albums_probs = []
        self._albums_choices_cache = None
        self._tracks = []
        self._tracks_probs = []
        self._tracks_choices_cache = None
        self._loaded_links_stats = {'artist': 0, 'album': 0, 'playlist': 0}
        self._skipped_links_stats = {'artist': 0, 'album': 0, 'playlist': 0}
        data = json.load(codecs.open(input, 'r', 'utf-8') if isinstance(input, basestring) else input)

        assert isinstance(data, list)

        sum_playlist_frequency = 0
        sum_artist_frequency = 0
        sum_album_frequency = 0
        sum_track_frequency = 0

        for element in data:
            tp = element.get('type')
            assert tp
            frequency = element.get('frequency')
            assert frequency is not None
            assert frequency >= 0
            frequency += 1

            identifier = str(element.get('id'))
            assert identifier
            identifier = (tp, identifier)

            if tp == 'artist':
                music_object = Artist(identifier, frequency=frequency)
            elif tp == 'playlist':
                is_editorial = element['is_editorial']
                music_object = Playlist(identifier, frequency=frequency, is_editorial=is_editorial)
            elif tp == 'album':
                music_object = Album(identifier, frequency=frequency)
            elif tp == 'track':
                music_object = Track(identifier, frequency=frequency)
            else:
                raise ValueError('Unknown type %s' % tp)

            self._music_objects[identifier] = music_object

        for element in data:
            tp = element.get('type')
            identifier = (tp, str(element.get('id')))
            music_object = self._music_objects.get(identifier)

            if not isinstance(music_object, Artist):
                continue

            assert tp == 'artist'

            label = element.get('label')
            assert label
            aliases = element.get('aliases')
            assert aliases is not None
            ruglish_aliases = element.get('ruglish_aliases')
            assert ruglish_aliases is not None
            anchors = element.get('anchors')
            assert anchors is not None

            labels, label_probs = self._prepare_labels(
                label, aliases, ruglish_aliases, anchors, music_object
            )

            if not labels:
                logger.error("Unable to load any label for object %s" % element)
                self._music_objects.pop(identifier)
                continue

            sum_artist_frequency += music_object.frequency
            self._artists.append(music_object)

            music_object.labels = labels
            music_object.label_probs = label_probs

        for element in data:
            tp = element.get('type')
            identifier = (tp, str(element.get('id')))
            music_object = self._music_objects.get(identifier)

            if not music_object or isinstance(music_object, Artist):
                continue

            assert tp != 'artist'

            if isinstance(music_object, Album):
                music_object.artists = self._read_music_object_list(
                    element.get('artists', []), Artist, music_object
                )
            elif isinstance(music_object, Track):
                music_object.artists = self._read_music_object_list(
                    element.get('artists', []), Artist, music_object
                )
                music_object.albums = self._read_music_object_list(
                    element.get('albums', []), Album, music_object
                )
                music_object.playlists = self._read_music_object_list(
                    element.get('playlists', []), Playlist, music_object
                )

            label = element.get('label')
            assert label
            aliases = element.get('aliases')
            assert aliases is not None
            ruglish_aliases = element.get('ruglish_aliases')
            assert ruglish_aliases is not None
            anchors = element.get('anchors')
            assert anchors is not None

            labels, label_probs = self._prepare_labels(
                label, aliases, ruglish_aliases, anchors, music_object
            )

            if not labels:
                logger.error("Unable to load any label for object %s" % element)
                self._music_objects.pop(identifier)
                continue

            if isinstance(music_object, Album):
                sum_album_frequency += music_object.frequency
                self._albums.append(music_object)
            elif isinstance(music_object, Track):
                sum_track_frequency += music_object.frequency
                self._tracks.append(music_object)
            else:
                sum_playlist_frequency += music_object.frequency
                self._playlists.append(music_object)

            music_object.labels = labels
            music_object.label_probs = label_probs

        self._playlists.sort(key=lambda x: -x.frequency)
        self._artists.sort(key=lambda x: -x.frequency)
        self._albums.sort(key=lambda x: -x.frequency)
        self._tracks.sort(key=lambda x: -x.frequency)

        for playlist in self._playlists:
            self._playlists_probs.append(float(playlist.frequency) / sum_playlist_frequency)

        for artist in self._artists:
            self._artists_probs.append(float(artist.frequency) / sum_artist_frequency)

        for album in self._albums:
            self._albums_probs.append(float(album.frequency) / sum_album_frequency)

        for track in self._tracks:
            self._tracks_probs.append(float(track.frequency) / sum_track_frequency)

        for music_object in self._music_objects.values():
            if isinstance(music_object, Album):
                music_object.artists = [a for a in music_object.artists if a.labels]
            elif isinstance(music_object, Track):
                music_object.artists = [a for a in music_object.artists if a.labels]
                music_object.albums = [a for a in music_object.albums if a.labels]
                music_object.playlists = [p for p in music_object.playlists if p.labels]

    def sample_random_playlist(self):
        obj = next(self._playlists_choices_cache, None) if self._playlists_choices_cache else None

        if obj:
            return obj

        self._playlists_choices_cache = iter(
            numpy.random.choice(self._playlists, p=self._playlists_probs, size=10000)
        )

        return next(self._playlists_choices_cache)

    def sample_random_artist(self):
        obj = next(self._artists_choices_cache, None) if self._artists_choices_cache else None

        if obj:
            return obj

        self._artists_choices_cache = iter(
            numpy.random.choice(self._artists, p=self._artists_probs, size=10000)
        )

        return next(self._artists_choices_cache)

    def sample_random_album(self):
        obj = next(self._albums_choices_cache, None) if self._albums_choices_cache else None

        if obj:
            return obj

        self._albums_choices_cache = iter(
            numpy.random.choice(self._albums, p=self._albums_probs, size=10000)
        )

        return next(self._albums_choices_cache)

    def sample_random_track(self):
        obj = next(self._tracks_choices_cache, None) if self._tracks_choices_cache else None

        if obj:
            return obj

        self._tracks_choices_cache = iter(
            numpy.random.choice(self._tracks, p=self._tracks_probs, size=10000)
        )

        return next(self._tracks_choices_cache)

    @staticmethod
    def _yield_objects(music_objects, object_probs, limit):
        for obj, obj_prob in izip(music_objects, object_probs):
            if limit is not None:
                limit -= 1
                if limit < 0:
                    break
            yield obj, obj_prob

    def yield_playlists(self, limit=None):
        for info in self._yield_objects(self._playlists, self._playlists_probs, limit):
            yield info

    def yield_artists(self, limit=None):
        for info in self._yield_objects(self._artists, self._artists_probs, limit):
            yield info

    def yield_albums(self, limit=None):
        for info in self._yield_objects(self._albums, self._albums_probs, limit):
            yield info

    def yield_tracks(self, limit=None):
        for info in self._yield_objects(self._tracks, self._tracks_probs, limit):
            yield info

    def get_loaded_links_stat(self):
        return self._loaded_links_stats.items()

    def get_skipped_links_stat(self):
        return self._skipped_links_stats.items()
