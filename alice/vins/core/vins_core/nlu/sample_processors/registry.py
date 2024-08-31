from __future__ import unicode_literals

import logging

from .base import BaseSampleProcessor
from .expected_value import ExpectedValueSampleProcessor
from .normalize import NormalizeSampleProcessor
from .misspell import MisspellSamplesProcessor
from .strip_activation import StripActivationSamplesProcessor
from .drop_censored import DropCensoredSamplesProcessor
from .clip import ClipSamplesProcessor
from .wizard import WizardSampleProcessor
from .entitysearch import EntitySearchSampleProcessor

_registry = {}

logger = logging.getLogger(__name__)


def register_sample_processor(sample_processor_type, name):
    assert issubclass(sample_processor_type, BaseSampleProcessor)
    sample_processor_type.NAME = name
    _registry[sample_processor_type.NAME] = sample_processor_type


def create_sample_processor(name, **kwargs):
    if name not in _registry:
        raise ValueError('Unknown sample processor: {}'.format(name))
    logger.debug("Creating {0} processor with params {1}".format(name, kwargs))
    return _registry[name](**kwargs)


def get_registered_processors():
    return list(_registry.keys())


register_sample_processor(ExpectedValueSampleProcessor, 'expected_value')
register_sample_processor(NormalizeSampleProcessor, 'normalizer')
register_sample_processor(MisspellSamplesProcessor, 'misspell')
register_sample_processor(StripActivationSamplesProcessor, 'strip_activation')
register_sample_processor(DropCensoredSamplesProcessor, 'drop_censored')
register_sample_processor(ClipSamplesProcessor, 'clip')
register_sample_processor(WizardSampleProcessor, 'wizard')
register_sample_processor(EntitySearchSampleProcessor, 'entitysearch')
