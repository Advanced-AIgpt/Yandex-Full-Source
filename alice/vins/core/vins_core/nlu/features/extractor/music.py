# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import logging
import numpy as np
from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, DenseFeatures

logger = logging.getLogger(__name__)


class MusicEntityFeatureData(object):
    def __init__(self, tokens, popularity_key):
        self._tokens = tokens
        self._popularity_key = popularity_key
        self._popularity = 0
        self._coverage = 0.0
        self._begin = -1
        self._end = len(tokens) + 1

    def update(self, occurrence):
        if len(self._tokens) == 0:
            return

        if occurrence[self._popularity_key] > self._popularity:
            self._popularity = occurrence[self._popularity_key]
            self._begin = occurrence['Begin']
            self._end = occurrence['End']

    def get_features(self):
        coverage = 0.0
        if self._popularity > 0 and len(self._tokens) > 0:
            coverage = float(self._end - self._begin) / len(self._tokens)
        return [
            self._popularity,
            coverage,
            self._popularity * coverage,
            self._begin,
            len(self._tokens) - self._end
        ]


class MusicFeaturesExtractor(BaseFeatureExtractor):
    def _call(self, sample, **kwargs):
        if 'wizard' not in sample.annotations:
            logger.warning('Wizard annotation not found in sample = "%s". Can\'t extract wizard features.', sample.text)
            return self.get_default_features(sample)

        wizard_annotation = sample.annotations['wizard']
        if 'MusicFeatures' not in wizard_annotation.rules:
            return self.get_default_features(sample)
        wizard_music_features = wizard_annotation.rules['MusicFeatures']

        if 'Tokens' not in wizard_music_features:
            return self.get_default_features(sample)
        tokens = wizard_music_features['Tokens']

        if 'Occurrences' not in wizard_music_features:
            return self.get_default_features()
        occurrences = wizard_music_features['Occurrences']

        artist_data = MusicEntityFeatureData(tokens, 'ArtistPopularity')
        album_data = MusicEntityFeatureData(tokens, 'AlbumPopularity')
        track_data = MusicEntityFeatureData(tokens, 'TrackPopularity')

        for occurrence in occurrences:
            artist_data.update(occurrence)
            album_data.update(occurrence)
            track_data.update(occurrence)

        return [DenseFeatures(data=np.array(
            artist_data.get_features() +
            album_data.get_features() +
            track_data.get_features()
        ))]

    def get_default_features(self, sample=None):
        return [DenseFeatures(data=np.array(
            MusicEntityFeatureData('', 'ArtistPopularity').get_features() +
            MusicEntityFeatureData('', 'AlbumPopularity').get_features() +
            MusicEntityFeatureData('', 'TrackPopularity').get_features()
        ))]

    @property
    def _features_cls(self):
        return DenseFeatures
