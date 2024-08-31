# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import attr


logger = logging.getLogger(__name__)


_annotation_types = {}
_annotation_ids = {}


def register_annotation(cls, id_):
    if id_ in _annotation_types:
        raise ValueError('Annotation with id="%s" is already registered.' % id_)

    _annotation_types[id_] = cls
    _annotation_ids[cls] = id_


def get_annotation_class(annotation_id):
    if annotation_id in _annotation_types:
        return _annotation_types[annotation_id]
    else:
        raise ValueError('Sample annotation type with id="%s" is not registered.' % annotation_id)


def get_annotation_id(cls):
    if cls in _annotation_ids:
        return _annotation_ids[cls]
    else:
        raise ValueError('Annotation class "%s" is not registered. Can\'t find id.' % cls)


def parse_annotation(data):
    annotation_id = data['type']
    return get_annotation_class(annotation_id).from_dict(data['value'])


class BaseAnnotation(object):
    """Class representing potentially important data about some text or dialogue turn. This data is called `annotation`.
    For example:
        * Named entities extracted from user query or VINS reply.
        * Wizard response for user query.
        * Objects/factoids from BASS for current dialogue turn.

    To store annotations, see :class:`vins_core.common.annotations.AnnotationsBag` documentation.

    Important Note: annotations are stored in SessionStorage, so if you want database entries to be space efficient,
        overload `trim` method for your particular annotation class.
    """
    @classmethod
    def from_dict(cls, data):
        return cls(data)

    def to_dict(self):
        return attr.asdict(self)

    def trim(self):
        """Drop annotation parts that are not needed in the future. This method is called before saving annotations
        into DB, so if you have annotation's parts needed only for current query, you can drop them here.

        Implement this method in your annotation class in order to make database entries more space efficient.

        By default, this method drops nothing.
        """
        pass


class AnnotationsBag(object):
    """Class representing key-value storage of (basestring, Annotation) pairs.
    It supports serialization & deserialization via to_dict/from_dict methods.

    Any :class:`vins_core.common.sample.Sample` object has `annotations` property with this type. For Sample's,
    we extract annotations with :mod:`vins_core.nlu.sample_processors.*` during samples extractor pipeline.


    """
    def __init__(self, bag=None):
        self._storage = bag or {}

    def add(self, key, value):
        if value is None:
            return

        if not isinstance(value, BaseAnnotation):
            raise ValueError("Expected BaseAnnotation child as value, found '%s'." % type(value))

        self._storage[key] = value

    def __setitem__(self, key, value):
        self.add(key, value)

    def __contains__(self, key):
        return key in self._storage

    def __getitem__(self, key):
        return self._storage[key]

    def get(self, key, default=None):
        return self._storage.get(key, default)

    def keys(self):
        return self._storage.keys()

    def iteritems(self):
        return self._storage.iteritems()

    def copy(self):
        return AnnotationsBag(bag=self._storage.copy())

    def clear(self):
        self._storage.clear()

    def update(self, other):
        for key, value in other.iteritems():
            self[key] = value

    def delete(self, key):
        self._storage.pop(key, None)

    def trim(self):
        """Trim all annotations in the bag. For details, see `BaseAnnotation.trim` method.
        """
        for annotation in self._storage.itervalues():
            annotation.trim()

    @classmethod
    def from_dict(cls, data, strict=True):
        """Deserialize AnnotationsBag from data which is in format:
        {
            "anaphora_annotation": { // stores serialized AnaphoraAnnotation object
                "type": "anaphora",
                "value": {
                    "resolved_string": "what was Stephen Hawking's age when he died?" // (answer is 76)
                }
            },
            ...
        }

        Parameters
        ----------
            data : dict
                Serialized AnnotationsBag.
            strict : bool, optional
                Flag indicating whether to raise exception on deserialization errors or no. Default is True.

        """
        if not data:
            return cls()

        res = cls()
        for k, v in data.iteritems():
            try:
                res[k] = parse_annotation(v)
            except ValueError as e:
                if strict:
                    raise

                logger.warning('Can\'t deserialize annotation "%s". Reason: "%s"', v, e.message)
        return res

    def to_dict(self):
        res = {}
        for key, annotation in self._storage.iteritems():
            res[key] = dict(type=get_annotation_id(type(annotation)), value=annotation.to_dict())

        return res
