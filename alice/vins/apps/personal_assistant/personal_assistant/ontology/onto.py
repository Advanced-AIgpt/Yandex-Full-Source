# coding: utf-8
from __future__ import unicode_literals

import logging

from vins_core.utils.data import load_data_from_file

NLG_KEY = 'nlg'
RELATIONS_KEY = 'relations'
INSTANCES_KEY = 'instances'
CHILD_ENTITIES_KEY = 'child_entities'
NAME_KEY = 'name'
TEXT_KEY = 'text'
WEIGHT_KEY = 'weight'
PHRASES_KEY = 'phrases'
PROPERTIES_KEY = 'properties'

logger = logging.getLogger(__name__)


class PhraseInfo(object):
    def __init__(self, text, weight=1.0, use_for_nlg=False):
        """
        :param text: text of the phrase
        :type text: unicode
        :param weight: importance of the phrase
        :type weight: float
        :param use_for_nlg: is it possible to use this phrase for nlg
        :type use_for_nlg: bool
        """
        self._text = text
        self._weight = weight
        if weight < 0:
            raise ValueError('weight can not be negative')
        self._use_for_nlg = use_for_nlg

    @classmethod
    def from_dict(cls, input_dict):
        return cls(input_dict[TEXT_KEY], input_dict.get(WEIGHT_KEY, 1.0), input_dict.get(NLG_KEY, False))

    @property
    def text(self):
        return self._text

    @property
    def weight(self):
        return self._weight

    @property
    def use_for_nlg(self):
        return self._use_for_nlg


class Entity(object):
    def __init__(self, name, ontology):
        """
        :param name: unique name of the entity
        :type name: unicode
        """
        self._name = name
        self._parent_entity = None  # type: Entity
        self._child_entities = {}  # type: dict[unicode,Entity]
        self._instances = {}  # type: dict[unicode,Entity.Instance]
        self._own_relations = {}  # type: dict[unicode,Entity]
        self._allowed_relations = None  # type: dict[unicode,Entity]
        self._ontology = ontology

    def add_relation(self, relation_name, range_entity):
        if relation_name in self._own_relations:
            raise ValueError('%s is already defined for %s' % (relation_name, self._name))
        self._own_relations[relation_name] = range_entity

    def add_child_entity(self, entity):
        """
        register child entity for the entity
        :type entity: Entity
        """
        if entity.name in self._child_entities:
            raise ValueError('child entity with name %s is already added to %s' % (entity.name, self._name))
        self._child_entities[entity.name] = entity
        entity._parent_entity = self

    def add_instance(self, instance):
        """
        register instance for the entity
        :type instance: Entity.Instance
        """
        if instance.name in self._instances:
            raise ValueError('entity instance with name %s is already added to %s' % (instance.name, self._name))
        self._instances[instance.name] = instance
        instance._parent_entity = self

    def is_derived_from(self, entity):
        """
        return True if this entity is descendant of another entity
        :param entity: ancestor entity
        :type entity: Entity
        :rtype: bool
        """
        if self._parent_entity is None:
            return False

        if self._parent_entity == entity:
            return True

        return self._parent_entity.is_derived_from(entity)

    @property
    def name(self):
        """
        :return: unique name of this entity
        :rtype: unicode
        """
        return self._name

    @property
    def parent_entity(self):
        """
        :return: parent entity for this entity
        :rtype: Entity
        """
        return self._parent_entity

    @property
    def child_entities(self):
        """
        :return: iterator of child entities
        """
        return self._child_entities.iterkeys()

    @property
    def instances(self):
        for e in self._child_entities.values():
            for i in e.instances:
                yield i

        for i in self._instances.values():
            yield i

    def _gather_relations_info(self, relations_map):
        """
        Gather information about relations allowed for the concept
        :param relations_map: target map (relation name -> range Entity)
        :type relations_map: dict[unicode,Entity]
        """
        if self._parent_entity is not None:
            self._parent_entity._gather_relations_info(relations_map)
        relations_map.update(self._own_relations)

    @property
    def allowed_relations(self):
        """
        Return map of relations allowed for this Entity
        :rtype: dict[unicode,Entity]
        """
        if self._allowed_relations is None:
            self._allowed_relations = {}
            self._gather_relations_info(self._allowed_relations)

        return self._allowed_relations

    def to_dict(self):
        child_entities = {}
        for ce in self._child_entities.values():
            child_entities.update(ce.to_dict())

        instances = {}
        for ci in self._instances.values():
            instances.update(ci.to_dict())

        body = {
            CHILD_ENTITIES_KEY: child_entities,
            INSTANCES_KEY: instances,
        }

        if self._own_relations:
            body[RELATIONS_KEY] = {x: r.name for x, r in self._own_relations.items()}

        return {self.name: body}

    class Instance(object):
        def __init__(self, name, ontology, phrases):
            """
            :param name: unique name of the entity instance
            :type name: unicode
            :param phrases: phrases that may be used for nlg or nlu
            :type phrases: list[PhraseInfo]
            """
            self._name = name

            if not phrases:
                raise ValueError('Instance %s must be provided with at least one phrase' % name)

            if not [p for p in phrases if p.use_for_nlg and p.weight > 0]:
                raise ValueError(
                    'At least one nlg phrase with positive weight must be specified for the instance %s' % name
                )

            self._phrases = tuple(phrases)
            self._parent_entity = None
            self._properties = {}
            self._ontology = ontology

        def add_property(self, rel_name, value):
            """
            Assign a property to the instance
            :param rel_name: property name
            :type rel_name: unicode
            :param value: value of the property
            :type value: Entity.Instance
            """
            if rel_name not in self._parent_entity.allowed_relations:
                raise ValueError('property with name %s is not allowed for instance %s of the entity %s' %
                                 (rel_name, self._name, self._parent_entity.name))
            rng = self._parent_entity.allowed_relations[rel_name]

            if rng == self._ontology[STRING_ENTITY_NAME]:
                if not isinstance(value, unicode):
                    raise ValueError(
                        'property with name %s of instance %s of the entity %s is expected to have '
                        'unicode string as value' %
                        (rel_name, self._parent_entity.name, self._name,))
            elif isinstance(value, basestring):
                value = self._ontology[value]
                if value is None or not isinstance(value, Entity.Instance):
                    raise ValueError('Range instance %s could not be found' % value)

                if not value.is_derived_from(rng):
                    raise ValueError('property with name %s and value %s is not allowed for instance %s of the '
                                     'entity %s (it must be derived from %s)' %
                                     (rel_name, value.name, self._name, self._parent_entity.name, rng.name))

            if rel_name not in self._properties:
                self._properties[rel_name] = value
            else:
                raise ValueError('Many-valued properties are not supported, property name is %s' % rel_name)

        def is_derived_from(self, entity):
            """
            return True if this entity instance belongs to the entity which is descendant of another entity
            :param entity: ancestor entity or name of ancestor entity
            :type entity: Entity or unicode
            :rtype: bool
            """

            if not isinstance(entity, Entity):
                entity = self._ontology[entity]
                if entity is None:
                    return False

            if self._parent_entity == entity:
                return True

            return self._parent_entity.is_derived_from(entity)

        def __getitem__(self, relation_name):
            """
            Return single value of a property if present, otherwise return None.
            Many-valued properties are not supported at the moment.
            """
            return self._properties.get(relation_name)

        @property
        def name(self):
            return self._name

        @property
        def parent_entity(self):
            return self._parent_entity

        @property
        def phrases(self):
            """
            :rtype: tuple[PhraseInfo]
            """
            return self._phrases

        def to_dict(self):
            phrases = []

            for pinfo in self._phrases:
                d = {TEXT_KEY: pinfo.text}
                w = pinfo.weight
                if w != 1.0:
                    d[WEIGHT_KEY] = w
                if pinfo.use_for_nlg:
                    d[NLG_KEY] = True

                phrases.append(d)

            body = {PHRASES_KEY: sorted(phrases, key=lambda ph: ph[TEXT_KEY])}

            if self._properties:
                body[PROPERTIES_KEY] = {
                    x: i.name if isinstance(i, Entity.Instance) else i
                    for x, i in self._properties.items()
                }

            return {self.name: body}


STRING_ENTITY_NAME = 'string_entity'


class Ontology(object):
    def __init__(self):
        # Map of all things (entities and entity instances)
        self._things = {}  # type: dict[unicode,object]
        # root entities
        self._roots = set()  # type: set[Entity]
        self._things[STRING_ENTITY_NAME] = Entity(STRING_ENTITY_NAME, self)

    @classmethod
    def from_file(cls, json_path):
        try:
            ontology_dict = load_data_from_file(json_path)
        except Exception as e:
            logger.error('Parse json failed with error: %s', e, exc_info=True)
            raise ValueError('Invalid json at file %s' % json_path)

        return Ontology.from_dict(ontology_dict)

    @classmethod
    def from_dict(cls, input_dict):
        """
        Read ontology from dictionary of following format:
        {
          "ParentEntityName": {
            "child_entities": {
              "ChildEntityName1": {
                "instances": {
                  ...
                }
                ...
              }
            },
            "instances": {
              "EntityInstanceName": {
                "phrases": [
                  {
                    "text": "phrase1",
                    "weight": <float>
                  },
                  {
                    "text": "phrase2",
                    "weight": <float>
                  },
                  ...
                ]
              },
            }
          }
          ...
        }
        :param input_dict: input dictionary
        :type input_dict: dict of (unicode,dict)
        :return: the ontology
        :rtype: Ontology
        """

        onto = cls()

        relations_to_resolve = []
        properties_to_resolve = []

        for (entity_name, entity_data) in input_dict.items():
            onto._roots.add(onto.read_entity(entity_name, entity_data, relations_to_resolve, properties_to_resolve))

        for (entity, rel_name, other_entity_name) in relations_to_resolve:
            other_entity = onto[other_entity_name]
            if other_entity is None:
                raise LookupError('Range entity %s not found' % other_entity_name)

            entity.add_relation(rel_name, other_entity)

        for (entity_instance, rel_name, property_value) in properties_to_resolve:
            entity_instance.add_property(rel_name, property_value)

        return onto

    def read_entity(self, name, input_dict, relations_to_resolve, properties_to_resolve):
        """
        Read entity from dictionary

        :param name: unique name of the entity
        :type name: unicode
        :param input_dict: input dictionary
        :type input_dict: dict[unicode,_]
        :param relations_to_resolve: relations that must be resolved on the second pass
        :type relations_to_resolve: list[tuple(Entity,unicode,unicode)]
        :param properties_to_resolve: properties that must be resolved on the second pass
        :type properties_to_resolve: list[tuple(Entity.Instance,unicode,unicode)]
        """
        if name in self._things:
            raise ValueError('Entity or entity instance %s is defined already' % name)

        e = Entity(name, self)
        self._things[name] = e

        for ce_name, ce_data in input_dict.get(CHILD_ENTITIES_KEY, {}).iteritems():
            e.add_child_entity(
                self.read_entity(
                    ce_name, ce_data, relations_to_resolve, properties_to_resolve
                )
            )

        for i_name, i_data in input_dict.get(INSTANCES_KEY, {}).iteritems():
            e.add_instance(self.read_instance(i_name, i_data, properties_to_resolve))

        for k, v in input_dict.get(RELATIONS_KEY, {}).items():
            relations_to_resolve.append((e, k, v))

        return e

    def read_instance(self, name, input_dict, properties_to_resolve):
        """
        Read instance from dictionary
        :param name: unique name of the instance
        :type name: unicode
        :param input_dict: input dictionary
        :type input_dict: dict[unicode,_]
        :param properties_to_resolve: properties that must be resolved on the second pass
        :type properties_to_resolve: list[tuple(Entity.Instance,unicode,unicode)]
        """
        if name in self._things:
            raise ValueError('Entity or entity instance %s is defined already' % name)

        phrases_arr = input_dict.get(PHRASES_KEY, [])
        if len(phrases_arr) == 0:
            raise ValueError('At least one phrase must be defined for entity or entity instance %s' % name)

        phrases = {}
        for phrase in phrases_arr:
            text = phrase[TEXT_KEY]
            if text in phrases:
                raise ValueError('Phrase with text %s is already present' % text)
            phrases[text] = PhraseInfo.from_dict(phrase)

        i = Entity.Instance(name, self, phrases.values())

        for k, v in input_dict.get(PROPERTIES_KEY, {}).iteritems():
            properties_to_resolve.append((i, k, v))

        self._things[name] = i
        return i

    @property
    def entities(self):
        for t in self._things:
            if type(t) == Entity:
                yield t

    @property
    def instances(self):
        for t in self._things:
            if type(t) == Entity.Instance:
                yield t

    @property
    def root_entites(self):
        for e in self._roots:
            yield e

    def __getitem__(self, item):
        return self._things.get(item)

    def export(self):
        return {key: val for d in (e.to_dict() for e in self.root_entites) for key, val in d.items()}
