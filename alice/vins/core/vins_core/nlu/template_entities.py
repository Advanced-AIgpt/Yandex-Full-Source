# coding: utf-8
from __future__ import unicode_literals

import logging

from vins_core.utils.data import open_resource_file

logger = logging.getLogger(__name__)


class TemplateEntitiesFormat(object):
    def __init__(self, name, filename=None, is_custom_entities=False, entities=None):
        if is_custom_entities:
            if entities is not None:
                self.data = self._parse_custom_entity_templates(entities)
            else:
                raise RuntimeError('Entities is None: %s' % name)
        else:
            self.data = self._parse_templates(filename)

    @staticmethod
    def _parse_templates(filename):
        result = []
        for line in open_resource_file(filename):
            line = line.strip()
            if line and not line.startswith('#'):
                result.append(line)
        return result

    @staticmethod
    def _parse_custom_entity_templates(entities):
        result = []
        for sample in entities.samples:
            for phrase in entities.samples[sample]:
                result.append(phrase.string)
        return result
