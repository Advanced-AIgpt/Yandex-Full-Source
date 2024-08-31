# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import attr

from collections import Iterable
from operator import itemgetter
from functools import partial

from vins_core.common.sample import Sample
from vins_core.dm.response import FeaturesExtractorErrorMeta
from vins_core.nlu.features.base import SampleFeatures, Features, FEATURE_CLASS_TO_NAME
from vins_core.nlu.features.extractor.classifier import ClassifierFeatureExtractor
from vins_core.nlu.features.extractor.embeddings import EmbeddingsMap, EmbeddingsFeaturesExtractor
from vins_core.nlu.features.extractor.embeddings_ids import EmbeddingsMapIds, EmbeddingsIdsFeaturesExtractor
from vins_core.nlu.features.extractor.morphology import LemmaFeatureExtractor, PosTagFeatureExtractor, \
    CaseFeatureExtractor
from vins_core.nlu.features.extractor.ner import NerFeatureExtractor
from vins_core.nlu.features.extractor.ngram import NGramFeatureExtractor, BagOfCharFeatureExtractor
from vins_core.nlu.features.extractor.serp import SerpFeatureExtractor
from vins_core.nlu.features.extractor.wizard import WizardFeatureExtractor
from vins_core.nlu.features.extractor.nlu_extra import NluExtraFeaturesExtractor
from vins_core.nlu.features.extractor.dssm.dssm_embeddings import DssmEmbeddingsFeaturesExtractor
from vins_core.nlu.features.extractor.regexp import RegexpFeatureExtractor
from vins_core.nlu.features.extractor.entitysearch import EntitySearchFeatureExtractor
from vins_core.nlu.features.extractor.music import MusicFeaturesExtractor
from vins_core.nlu.features.extractor.granet import GranetFeatureExtractor
from vins_core.utils.config import get_setting
from vins_core.utils.misc import parallel


logger = logging.getLogger(__name__)


@attr.s
class ExtractorInfo(object):
    extractor = attr.ib()
    params = attr.ib()

    def __call__(self, *args, **kwargs):
        return self.extractor(*args, **kwargs)

    def get_default_features(self, sample):
        return self.extractor.get_default_features(sample)


class FeaturesExtractorFactory(object):
    """
    This class instantiates features extractors based on input parameters.
    The following features are available:
    <feature name> : <feature parameter, type>
    - ngrams: <int>
    - ner: {'parser': <parser name, string>, 'custom_entity_parsers': <use custom ners, bool>
    - wizard: <use wizard, bool> or <wizard params, dict>
    - lemma: <use lemmas for words, bool>
    - postag: <use part-of-speech tags, bool>
    - case: <use cases, bool>
    - serp: <use SERP features, bool>
    - classifier: <3rd party classifier parameters, dict>
    - embeddings: {'file': <embeddings file, string> } or {'resource': <embedding sandbox resource, string> }
    """

    def __init__(self):
        self._extractors = {}
        self._classifiers_info = {}
        self._parser = None

        self._create_extractor = {
            'ngrams': self._get_ngram,
            'ner': self._get_ner,
            'wizard': self._get_wizard,
            'nlu_extra': self._get_nlu_extra_features,
            'lemma': self._get_lemma,
            'postag': self._get_postag,
            'case': self._get_case,
            'serp': self._get_serp,
            'classifier': self._get_classifiers,
            'embeddings': partial(self._get_embeddings, EmbeddingsMap, EmbeddingsFeaturesExtractor),
            'embeddings_ids': partial(self._get_embeddings, EmbeddingsMapIds, EmbeddingsIdsFeaturesExtractor),
            'dssm_embeddings': self._get_dssm_embeddings,
            'bagofchar': self._get_bagofchar,
            'regexp': self._get_regexp,
            'entitysearch': self._get_entitysearch,
            'music_features': self._get_music_features,
            'granet': self._get_granet
        }

    def register_parser(self, parser):
        self._parser = parser

    def _validate_new_extractor(self, id, type, **params):
        if id in self._extractors and params != self._extractors[id].params:
            raise ValueError(
                'Found several feature extractors with id="%s" and different parameters. Check config.' % id
            )
        elif type not in self._create_extractor:
            raise ValueError(
                'Unknown feature extractor "%r"' % type
            )

    def add(self, id, type, **params):
        """
        Add new feature extractor based on its parameters.
        :param id:
        :param type:
        :param params:
        :return: extractor_id: extractor identifier,
        all feature names related to the extractor has corresponding extractor_id prefix
        """
        self._validate_new_extractor(id, type, **params)

        logger.info('Adding feature extractor id="%s", type="%s" with parameters "%r"' % (id, type, params))

        extractor_info = self._create_extractor[type](params)
        self._extractors[id] = extractor_info
        self._classifiers_info.update(extractor_info.extractor.get_classifiers_info())

    def create_extractor(self):
        return FeaturesExtractor(self.extractors, self._classifiers_info)

    def reset(self):
        self._extractors = {}
        self._custom_entities = None

    @property
    def extractors(self):
        return self._extractors

    @property
    def features(self):
        return sorted(self._create_extractor.keys())

    @classmethod
    def _get_ngram(cls, params):
        # N-grams
        return ExtractorInfo(NGramFeatureExtractor(**params), params)

    def _get_ner(self, params):
        # Named entities
        new_params = params.copy()
        new_params['parser'] = self._parser
        return ExtractorInfo(NerFeatureExtractor(**new_params), params)

    @classmethod
    def _get_wizard(cls, params):
        # Wizard named entities
        return ExtractorInfo(WizardFeatureExtractor(**params), params)

    @classmethod
    def _get_lemma(cls, params):
        # Lemmas
        return ExtractorInfo(LemmaFeatureExtractor(), params)

    @classmethod
    def _get_postag(cls, params):
        # Part-of-speech tags
        return ExtractorInfo(PosTagFeatureExtractor(), params)

    @classmethod
    def _get_case(cls, params):
        # Case
        return ExtractorInfo(CaseFeatureExtractor(), params)

    @classmethod
    def _get_serp(cls, params):
        # Serp
        return ExtractorInfo(SerpFeatureExtractor(**params), params)

    @classmethod
    def _get_regexp(cls, params):
        # Regexp
        return ExtractorInfo(RegexpFeatureExtractor(**params), params)

    @classmethod
    def _get_classifiers(cls, params):
        # Predictions from side classifiers
        from vins_core.nlu.nontrainable_token_classifier import create_nontrainable_token_classifier
        from vins_core.nlu.token_classifier import create_token_classifier
        extractor_params = {
            param: value for param, value in params.iteritems()
            if param in ('feature',)
        }
        token_classifier = None
        if 'model_file' in params:
            token_classifier = create_nontrainable_token_classifier(config=params, **params)
        if token_classifier is None:
            token_classifier = create_token_classifier(config=params, **params)
        return ExtractorInfo(ClassifierFeatureExtractor(
            token_classifier=token_classifier,
            **extractor_params
        ), params)

    @classmethod
    def _get_embeddings(cls, emap, eextractor, params):
        # Embeddings
        if not params.get('file') and not params.get('resource'):
            raise ValueError(
                'Wrong parameter specified for %s; the following is expected: %s' %
                (params['feature_id'], '{"file": <filename>} or {"resource": <sandbox resource id>}')
            )
        if params.get('file'):
            embeddings_map = emap.load_from_bin_file(params['file'])
        else:
            embeddings_map = emap.load_from_bin_resource(params['resource'])

        return ExtractorInfo(
            eextractor(embeddings_map=embeddings_map, **params),
            params
        )

    @classmethod
    def _get_dssm_embeddings(cls, params):
        return ExtractorInfo(DssmEmbeddingsFeaturesExtractor(**params), params)

    @classmethod
    def _get_bagofchar(cls, params):
        return ExtractorInfo(BagOfCharFeatureExtractor(**params), params)

    @classmethod
    def _get_entitysearch(cls, params):
        return ExtractorInfo(EntitySearchFeatureExtractor(), params)

    @classmethod
    def _get_music_features(cls, params):
        return ExtractorInfo(MusicFeaturesExtractor(), params)

    @classmethod
    def _get_nlu_extra_features(cls, params):
        return ExtractorInfo(NluExtraFeaturesExtractor(), params)

    @classmethod
    def _get_granet(cls, params):
        return ExtractorInfo(GranetFeatureExtractor(), params)


class FeaturesExtractor(object):
    """
    This class wraps multiple feature extractors in one sklearn-compatible pipeline step
    Specified features will be stacked for each input token
    """
    def __init__(self, extractors, classifiers_info):
        self._extractors = extractors
        self._classifiers_info = classifiers_info

    @property
    def classifiers_info(self):
        return self._classifiers_info

    @property
    def signature(self):
        result = {feature_type: [] for feature_type in list(Features)}

        for extractor_id, extracto_info in self._extractors.iteritems():
            for features_cls in extracto_info.extractor.features_clss:
                result[FEATURE_CLASS_TO_NAME[features_cls]].append(extractor_id)

        return result

    def get_sample_features(self, sample, response, **kwargs):
        sample_features = SampleFeatures(sample)
        for feature_id, extractor in self._extractors.iteritems():
            try:
                features = extractor(sample, **kwargs)
            except Exception as e:
                logger.warning('Features extractor %s failed with error: %s', feature_id, e, exc_info=True)
                features = extractor.get_default_features(sample)
                if response:
                    response.add_meta(FeaturesExtractorErrorMeta(features_extractor=feature_id))
            for feature in features:
                sample_features.add(feature, feature_id)
        return sample_features

    def _call_one(self, sample, response=None, **kwargs):
        return self.get_sample_features(sample, response, **kwargs)

    # FIXME: DIALOG-4035 may be use feature cache here
    def __call__(self, samples, num_procs=None, feature_cache=None, **kwargs):
        """
        Gets list of Sample instances and returns batch of features, with TokenFeatures per sample's token
        :param samples: iterable of Sample objects
        :param num_procs (int, optional) number of concurrent processes, or None to use environmental var VINS_NUM_PROCS
        :param feature_cache (string, optional) path to file where to store / retrieve cached samples
        :param kwargs: any arguments passed to feature extractors
        :return: list of SampleFeatures
        """
        assert isinstance(samples, Iterable)
        assert all(isinstance(sample, Sample) for sample in samples)
        features = parallel(
            function=self._call_one,
            function_kwargs=kwargs,
            items=samples,
            num_procs=num_procs,
        )
        self._logging(features)
        return features

    @classmethod
    def _logging(cls, batch_features):
        if not get_setting('FEATURE_DEBUG', False):
            return
        out = ''
        for features in batch_features:
            out += unicode(features)
        logger.debug(out)


def create_features_extractor(parser=None, **params):
    factory = FeaturesExtractorFactory()
    factory.register_parser(parser)
    for name, value in sorted(params.items(), key=itemgetter(0)):
        factory.add(name, name, **value)
    return factory.create_extractor()
