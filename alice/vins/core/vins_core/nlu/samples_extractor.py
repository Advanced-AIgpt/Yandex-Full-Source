# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.common.sample import Sample
from vins_core.common.utterance import Utterance
from vins_core.dm.formats import NluSourceItem, NluWeightedString
from vins_core.nlu.sample_processors.registry import create_sample_processor
from vins_core.utils.misc import parallel

logger = logging.getLogger(__name__)


class SamplesExtractorError(ValueError):
    pass


class SamplesExtractor(object):
    """Class which is responsible for extracting samples from different source types (see ``from_*`` methods of
    Sample class) and process them through specified pipeline and return Sample instance.

    Usage:
        from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor

        pipeline = [NormalizeSampleProcessor('normalizer_ru')]
        extractor = SamplesExtractor(pipeline=pipeline)
        sample1, sample2 = extractor(['hello world', 'how are you?'])

    """
    def __init__(self, pipeline=None, allow_wizard_request=None):
        """
        Parameters
        ----------
        pipeline : list of BaseSampleProcessor, optional
            List of sample processors which to use while obtaining result Sample.
            If no processors are passed, Sample.from_string(item) is returned.

        """
        self._pipeline = pipeline or []
        self._allow_wizard_request = allow_wizard_request

    def set_allow_wizard_request(self, value):
        self._allow_wizard_request = value

    def add(self, processor):
        self._pipeline.append(processor)

    def process_item(self, item, session, is_inference, app_id='', normalize_only=False, sample_processor_names=None,
                     **kwargs):
        sample = self._create_sample(item, app_id)
        return self._process_sample(sample, session, is_inference, normalize_only, sample_processor_names, **kwargs)

    @staticmethod
    def _skip_processor(processor, normalize_only, sample_processor_names):
        if normalize_only and not processor.is_normalizing:
            return True
        if sample_processor_names is not None and processor.NAME not in sample_processor_names:
            return True
        return False

    def _process_sample(self, processed_sample, session, is_inference, normalize_only, sample_processor_names,
                        **kwargs):
        self._check_sample_processor_names(sample_processor_names)

        for processor in self._pipeline:
            if self._skip_processor(processor, normalize_only, sample_processor_names):
                continue
            processed_sample = processor(processed_sample, session, is_inference, **kwargs)

        # Save sample annotations in session.
        if session and session.annotations:
            session.annotations.update(processed_sample.annotations)
        elif session:
            session.annotations = processed_sample.annotations.copy()

        return processed_sample

    def _check_sample_processor_names(self, sample_processor_names):
        if sample_processor_names is None:
            return

        if len(sample_processor_names) == 0:
            logger.warning(
                'SamplesExtractor was provided with empty set as "sample_processor_names". '
                'No sample processors will be applied.'
            )
            return

        full_pipeline = set(self.pipeline)
        for processor_name in sample_processor_names:
            if processor_name not in full_pipeline:
                raise SamplesExtractorError(
                    'Sample processor "{}" is not a part of full pipeline {}'.format(processor_name, self.pipeline)
                )

    def _call_one(self, item, session, is_inference, app_id, normalize_only, sample_processor_names, request_id=None,
                  **kwargs):
        return self.process_item(item, session, is_inference, app_id, normalize_only, sample_processor_names,
                                 request_id=request_id, **kwargs)

    # FIXME: DIALOG-4035 may be use feature cache here
    def __call__(self, items, session=None, is_inference=True, num_procs=None, sample_cache=None, app_id='',
                 normalize_only=False, sample_processor_names=None, request_id=None, features=None,
                 not_use_wizard_from_mm_flag=None, not_use_entity_from_mm_flag=None, **kwargs):
        """Extract samples from `items`.

        Parameters
        ----------
        items : {iterable of vins_core.nlu.formats.NluSourceItem, vins_core.dm.utterance.Utterance, str, unicode, None}
            Items which are to be parsed.
        session : vins_core.dm.session.Session, optional
            Session object. Default is None.
        is_inference : bool, optional
            Flag indicating whether it's inference or training/initialization phase. Default is True.
        num_procs : int, optional
            Number of concurrent processes, or None to use environment variable VINS_NUM_PROCS.
        sample_cache : string, optional
            Path to file where to store / retrieve cached samples.
        update_sample_cache : bool, optional
            Whether to update or create new sample cache.
        normalize_only : bool, optional
            Whether to skip non-normalizing processors
        sample_processor_names : iterable of strings, optional
            Names of sample processors to apply extraction pipeline partially, or None to apply full pipeline.

        Returns
        -------
        samples : list of Sample

        """
        function_kwargs = {
            'session': session,
            'is_inference': is_inference,
            'app_id': app_id,
            'normalize_only': normalize_only,
            'sample_processor_names': sample_processor_names,
            'request_id': request_id,
            'features': features,
            'not_use_wizard_from_mm_flag': not_use_wizard_from_mm_flag,
            'not_use_entity_from_mm_flag': not_use_entity_from_mm_flag,
            'allow_wizard_request': self._allow_wizard_request
        }

        samples = parallel(
            function=self._call_one,
            function_kwargs=function_kwargs,
            items=items,
            num_procs=num_procs,
            **kwargs
        )
        if not samples:
            samples = [self._create_sample(None, app_id=app_id) for _ in items]

        return samples

    @classmethod
    def _create_sample(cls, item, app_id=''):
        if isinstance(item, Sample):
            sample = item
        elif isinstance(item, NluSourceItem):
            sample = Sample.from_nlu_source_item(item)
        elif isinstance(item, Utterance):
            sample = Sample.from_utterance(item)
        elif isinstance(item, basestring):
            sample = Sample.from_string(item)
        elif isinstance(item, NluWeightedString):
            sample = Sample.from_weighted_string(item)
        elif item is None:
            sample = Sample.from_none()
        else:
            raise RuntimeError(
                'Input items type is not recognized, should be one of: '
                'vins_core.nlu.formats.NluSourceItem, str, unicode or None'
            )
        sample.app_id = app_id
        return sample

    @property
    def pipeline(self):
        return [p.NAME for p in self._pipeline]

    @classmethod
    def from_config(cls, config):
        if not config:
            return cls.default_samples_extractor()

        pipeline_config = config.get('pipeline', [])

        pipeline = []
        for processor_config in pipeline_config:
            if 'name' not in processor_config:
                raise RuntimeError('Invalid json for processor. "name" must be specified. '
                                   'You passed {}'.format(processor_config))
            else:
                logger.info('Adding samples extractor %s', processor_config['name'])
                pipeline.append(create_sample_processor(**processor_config))

        return cls(pipeline=pipeline, allow_wizard_request=config.get('allow_wizard_request'))

    @classmethod
    def default_samples_extractor(cls):
        return cls()
