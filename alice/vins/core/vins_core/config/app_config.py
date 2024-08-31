# coding: utf-8
from __future__ import unicode_literals

import attr
import copy
import logging
import os

import jsonschema

from collections import defaultdict, Mapping, Iterable

from vins_core.config import schemas
from vins_core.dm.form_filler.models import Form
from vins_core.nlg.template_nlg import create_simple_phrases, create_branched_phrase
from vins_core.ner.fst_normalizer import NluFstNormalizer
from vins_core.nlu.template_entities import TemplateEntitiesFormat
from vins_core.utils import archives
from vins_core.utils.data import load_data_from_file, get_resource_full_path
from vins_core.dm.formats import NluWeightedString

logger = logging.getLogger(__name__)


def _get_data_or_filepath_or_include(item):
    if 'data' in item:
        yield item['data'].copy(), None
    elif 'path' in item:
        yield None, item['path']
    elif 'paths' in item:
        for item_path in item['paths']:
            yield None, item_path


def _convert_sample_values(samples):
    res = defaultdict(list)
    for key, values in samples.iteritems():
        for syn in values:
            do_drop = False
            val, weight = syn, 1.
            if isinstance(syn, (list, tuple)) and isinstance(syn[0], basestring) and isinstance(syn[1], float):
                val, weight = syn
            elif not isinstance(syn, basestring):
                do_drop = True

            if do_drop or NluFstNormalizer.has_unknown_symbol(val):
                logger.info('Dropping value "%s" for key "%s" while constructing custom entity.', syn, key)
                continue

            res[key].append(NluWeightedString(val, weight))

    return res


@attr.s
class EntityInflectInfo(object):
    inflect_numbers = attr.ib(default=False)
    inflect = attr.ib(default=True)


@attr.s
class Entity(object):
    name = attr.ib()
    samples = attr.ib(repr=False, converter=_convert_sample_values)
    inflect_info = attr.ib(default=attr.Factory(EntityInflectInfo))
    use_as_template = attr.ib(default=False)

    @classmethod
    def from_dict(cls, obj):
        """
        {
          "entity": "custom_currency",
          "path": "entities/custom_currency.js"
          "include": "some_app://entities/custom_currency.js" // path or include required
          "inflect_numbers": true //optional
          "inflect": true //optional
        }
        """
        name = obj['entity']
        file_path = obj['path']

        samples = load_data_from_file(file_path)
        use_as_template = obj.get('use_as_template', False)
        inflect_numbers = obj.get('inflect_numbers', False)
        inflect = obj.get('inflect', True)

        inflect_info = EntityInflectInfo(inflect_numbers=inflect_numbers, inflect=inflect)
        return cls(
            name=name,
            samples=samples,
            inflect_info=inflect_info,
            use_as_template=use_as_template
        )


@attr.s
class NluSourcesConfig(object):
    config = attr.ib()


class Intent(object):
    @classmethod
    def from_dict(cls, obj, base_name=None):
        """
        {
          "intent": "get_weather",
          "prior": 1.0,
          "fallback": false,
          "reset_form": false,
          "nlu": {
            "path": "intents/get_weather.nlu",
            ...
            "data": [
                "что там с погодой",
                "погода в питере"
            ]
          },
          "dm": {
            "path": "intents/get_weather.js"
          },
          "nlg": {
            "path": "intents/get_weather.nlg"
          },
          "form_filling": {
            "max_intents": 20
          }
        }
        """
        name = obj['intent']
        parent = obj.get('parent')
        parent_examples_in_tagger = obj.get('parent_examples_in_tagger', True)
        prior = obj.get('prior', 1.0)
        fallback = obj.get('fallback', False)
        fallback_threshold = obj.get('fallback_threshold', None)
        total_fallback = obj.get('total_fallback', False)
        reset_form = obj.get('reset_form', False)
        trainable_classifiers = obj.get('trainable_classifiers')
        positive_sampling = obj.get('positive_sampling', True)
        negative_sampling_from = obj.get('negative_sampling_from', None)
        negative_sampling_for = obj.get('negative_sampling_for')
        scenarios = obj.get('scenarios')
        utterance_tagger = obj.get('utterance_tagger')

        if base_name:
            name = '{0}.{1}'.format(base_name, name)
        if base_name and parent:
            parent = '{0}.{1}'.format(base_name, parent)

        nlu = NluSourcesConfig(config=obj.get('nlu', []))

        nlg_data = None
        nlg_filename = None
        if 'nlg' in obj:
            nlg_data, nlg_filename = next(_get_data_or_filepath_or_include(obj['nlg']))

        dm = None
        if 'dm' in obj:
            data, filepath = next(_get_data_or_filepath_or_include(obj['dm']))
            if filepath:
                data = load_data_from_file(filepath)
            dm = Form.from_dict(data)
            if dm:
                dm.name = name

        return cls(
            name=name,
            nlu_sources=nlu,
            nlg_sources=nlg_data,
            nlg_filename=nlg_filename,
            dm_form=dm,
            parent=parent,
            parent_examples_in_tagger=parent_examples_in_tagger,
            prior=prior,
            fallback=fallback,
            fallback_threshold=fallback_threshold,
            total_fallback=total_fallback,
            reset_form=reset_form,
            trainable_classifiers=trainable_classifiers,
            positive_sampling=positive_sampling,
            negative_sampling_from=negative_sampling_from,
            negative_sampling_for=negative_sampling_for,
            scenarios=scenarios,
            utterance_tagger=utterance_tagger,
            suggests=obj.get('suggests'),
            submit_form=obj.get('submit_form'),
            allowed_prev_intents=obj.get('allowed_prev_intents'),
            allowed_device_states=obj.get('allowed_device_states'),
            allowed_apps=obj.get('allowed_apps'),
            experiments=obj.get('experiments'),
            allowed_active_slots=obj.get('allowed_active_slots'),
            request_filter=obj.get('request_filter'),
        )

    def __init__(
            self,
            name,
            nlu_sources=None,
            dm_form=None,
            nlg_sources=None,
            nlg_filename=None,
            parent=None,
            parent_examples_in_tagger=None,
            prior=1.0,
            fallback=False,
            fallback_threshold=None,
            total_fallback=False,
            reset_form=False,
            trainable_classifiers=None,
            positive_sampling=True,
            negative_sampling_from=None,
            negative_sampling_for=None,
            scenarios=None,
            utterance_tagger=None,
            suggests=None,
            submit_form=None,
            allowed_prev_intents=None,
            allowed_device_states=None,
            allowed_apps=None,
            experiments=None,
            gc_fallback=False,
            allowed_active_slots=None,
            request_filter=None,
            show_led_gif=None,
            gc_microintent=False,
            gc_swear_microintent=False
    ):
        # todo: move the validation into schemas.py and make sure it works there
        # todo: maybe compile the regular expressions here
        NoneType = type(None)
        if scenarios is not None:
            for scenario in scenarios:
                assert isinstance(scenario, Mapping)
                assert 'name' in scenario and isinstance(scenario['name'], basestring)
                context = scenario.get('context')
                if context is not None:
                    assert isinstance(context.get('app'), (basestring, NoneType))
                    assert isinstance(context.get('device_state'), (Mapping, NoneType))
                    assert isinstance(context.get('experiments'), (basestring, NoneType))
                    assert isinstance(context.get('prev_intents'), (basestring, NoneType))
                    assert isinstance(context.get('active_slots'), (Iterable, bool, NoneType))
                    assert not set(context.keys()).difference(
                        {'app', 'device_state', 'experiments', 'prev_intents', 'active_slots', 'request_filter'}
                    )
        assert isinstance(allowed_prev_intents, (Iterable, basestring, NoneType))
        assert isinstance(allowed_device_states, (Mapping, NoneType))
        assert isinstance(allowed_apps, (basestring, NoneType))
        assert isinstance(experiments, (basestring, NoneType))
        assert isinstance(allowed_active_slots, (Iterable, bool, NoneType))

        self.name = name
        self._nlu_sources = nlu_sources
        self.nlg_sources = nlg_sources
        self.nlg_filename = nlg_filename
        self._dm_form = dm_form
        self._parent = parent
        self._parent_examples_in_tagger = parent_examples_in_tagger
        self._prior = prior
        self._fallback = fallback
        self._fallback_threshold = fallback_threshold
        self._total_fallback = total_fallback
        self._reset_form = reset_form
        self._trainable_classifiers = trainable_classifiers
        self._positive_sampling = positive_sampling
        self._negative_sampling_from = negative_sampling_from
        self._negative_sampling_for = negative_sampling_for
        self._scenarios = scenarios
        self._utterance_tagger = utterance_tagger
        self._suggests = suggests
        self._submit_form = submit_form
        self._allowed_prev_intents = allowed_prev_intents
        self._allowed_device_states = allowed_device_states
        self._allowed_apps = allowed_apps
        self._experiments = experiments
        self._gc_fallback = gc_fallback
        self._allowed_active_slots = allowed_active_slots
        self._request_filter = request_filter
        self._show_led_gif = show_led_gif
        self._gc_microintent = gc_microintent
        self._gc_swear_microintent = gc_swear_microintent

    @property
    def nlu(self):
        return self._nlu_sources

    @property
    def nlg(self):
        return self.nlg_sources

    @property
    def dm(self):
        return self._dm_form

    @property
    def parent(self):
        return self._parent

    @property
    def parent_examples_in_tagger(self):
        return self._parent_examples_in_tagger

    @property
    def prior(self):
        return self._prior

    @property
    def fallback(self):
        return self._fallback

    @property
    def fallback_threshold(self):
        return self._fallback_threshold

    @property
    def total_fallback(self):
        return self._total_fallback

    @property
    def reset_form(self):
        return self._reset_form

    @property
    def trainable_classifiers(self):
        return self._trainable_classifiers

    @trainable_classifiers.setter
    def trainable_classifiers(self, value):
        self._trainable_classifiers = copy.copy(value)

    @property
    def positive_sampling(self):
        return self._positive_sampling

    @property
    def negative_sampling_from(self):
        return self._negative_sampling_from

    @property
    def negative_sampling_for(self):
        return self._negative_sampling_for

    @property
    def scenarios(self):
        return self._scenarios

    @property
    def utterance_tagger(self):
        return self._utterance_tagger

    @property
    def suggests(self):
        return self._suggests

    @property
    def submit_form(self):
        return self._submit_form

    @property
    def allowed_prev_intents(self):
        return self._allowed_prev_intents

    @property
    def allowed_device_states(self):
        return self._allowed_device_states

    @property
    def allowed_apps(self):
        return self._allowed_apps

    @property
    def experiments(self):
        return self._experiments

    @property
    def gc_fallback(self):
        return self._gc_fallback

    @property
    def allowed_active_slots(self):
        return self._allowed_active_slots

    @property
    def request_filter(self):
        return self._request_filter

    @property
    def show_led_gif(self):
        return self._show_led_gif

    @property
    def gc_microintent(self):
        return self._gc_microintent

    @property
    def gc_swear_microintent(self):
        return self._gc_swear_microintent


class MicrointentStorage(object):

    DEFAULT_NLG_PHRASE_ID = 'response'
    INHERITABLE_SETTINGS = [
        'allowed_prev_intents',
        'allowed_device_states',
        'allowed_apps',
        'experiments',
        'allowed_active_slots',
        'scenarios'
    ]

    def __init__(self, intents=None):
        self._intents = intents or []

    def iter(self):
        return iter(self._intents)

    @classmethod
    def _get_nlg_phrase_id(cls, cfg):
        return cfg.get('nlg_phrase_id', cls.DEFAULT_NLG_PHRASE_ID)

    @classmethod
    def _get_form_submit_handler(cls, cfg):
        DEFAULT_FORM_SUBMIT_HANDLER = {
            'handler': 'callback',
            'name': 'nlg_callback',
            'params': {
                'phrase_id': cls._get_nlg_phrase_id(cfg)
            }
        }
        return cfg.get('form_submit_handler', DEFAULT_FORM_SUBMIT_HANDLER)

    @classmethod
    def _get_fallback_threshold(cls, cfg):
        fallback_threshold = cfg.get('fallback_threshold')
        if fallback_threshold is None or isinstance(fallback_threshold, (float, int)):
            fallback_threshold = {'default': fallback_threshold}
        elif isinstance(fallback_threshold, dict):
            if not fallback_threshold.get('default'):
                raise ValueError('Fallback thresholds dict must contain "default" key.')
        else:
            raise ValueError('Wrong type of "fallback_threshold" key. '
                             'Must be float or dict. Passed %s' % type(fallback_threshold))

        return fallback_threshold

    @classmethod
    def iter_nlu_nlg(cls, storage_path, base_name=None, nlg_phrase_id=None, nlg_includes=(), nlg_checks=None):
        nlg_phrase_id = nlg_phrase_id or cls.DEFAULT_NLG_PHRASE_ID
        microintent_data = load_data_from_file(storage_path)
        schema = add_checks_to_microintents_schema(schemas.microintent_item_schema, nlg_checks)
        for intent_name, intent_config in microintent_data.iteritems():
            jsonschema.validate(intent_config, schema)
            if base_name:
                intent_name = '{0}.{1}'.format(base_name, intent_name)
            if 'nlu' not in intent_config:
                raise ValueError('Micro-intent %s does not have NLU phrases' % intent_name)

            nlu = NluSourcesConfig(config=[{'source': 'data', 'data': intent_config['nlu']}])

            nlg = cls._get_nlg(nlg_phrase_id, intent_config, nlg_includes)
            yield intent_name, nlu, nlg, intent_config

    @staticmethod
    def _get_nlg(nlg_phrase_id, intent_config, nlg_includes=()):
        nlg = None
        if 'nlg' in intent_config:
            nlg_config = intent_config['nlg']
            mode = 'cycle'
            if intent_config.get('allow_repeats', False):
                mode = None
            elif intent_config.get('gc_fallback', False):
                mode = 'no_repeat'
            if isinstance(nlg_config, dict):
                nlg = create_branched_phrase(nlg_phrase_id, nlg_config, nlg_includes, mode)
            else:
                nlg = create_simple_phrases(
                    phrases=[
                        (nlg_phrase_id, nlg_config)
                    ],
                    includes=nlg_includes,
                    mode=mode
                )
        return nlg

    @classmethod
    def from_dict(cls, obj, base_name=None, nlg_checks=None):
        storage_path = obj['path']
        fallback_threshold = cls._get_fallback_threshold(obj)
        trainable_classifiers = obj.get('trainable_classifiers')
        default_fallback_threshold = fallback_threshold.get('default')  # Float or None.
        nlg_phrase_id = cls._get_nlg_phrase_id(obj)
        nlg_includes = obj.get('nlg_includes', [])
        form_submit_handler = cls._get_form_submit_handler(obj)
        if 'prefix' in obj and base_name is not None:
            base_name = '{0}.{1}'.format(base_name, obj['prefix'])

        default_config = dict()
        for key in cls.INHERITABLE_SETTINGS:
            if key in obj:
                default_config[key] = obj[key]

        intents = []
        for intent_name, nlu, nlg, intent_config in cls.iter_nlu_nlg(storage_path,
                                                                     base_name,
                                                                     nlg_phrase_id,
                                                                     nlg_includes, nlg_checks=nlg_checks):
            intent_threshold = fallback_threshold.get(intent_name, default_fallback_threshold)

            form = Form.from_dict({
                'name': intent_name,
                'events': [{
                    'name': 'submit',
                    'handlers': [
                        form_submit_handler
                    ]
                }]
            })

            new_config = copy.copy(default_config)
            new_config.update(intent_config)
            # 'nlu' is a required field, so there is always something to delete
            # all the other fields are free to stay, as long as Intent constructor supports them
            del new_config['nlu']
            if 'nlg' in new_config:
                del new_config['nlg']
            if 'allow_repeats' in new_config:
                del new_config['allow_repeats']
            new_config['nlu_sources'] = nlu
            new_config['nlg_sources'] = nlg
            new_config['nlg_filename'] = storage_path
            new_config['dm_form'] = form
            new_config['fallback_threshold'] = intent_threshold
            new_config['trainable_classifiers'] = trainable_classifiers

            intents.append(Intent(intent_name, **new_config))
        return cls(intents)


def get_nlu_data_from_microintents(storage_path):
    return {
        intent: nlu.config[0]['data']
        for intent, nlu, nlg, intent_config in MicrointentStorage.iter_nlu_nlg(storage_path)
    }


class Project(object):
    @classmethod
    def from_dict(cls, obj, base_name=None, nlu_templates=None, nlg_checks=None):
        """
        {
          "name": "weather",
          "microintents" [
            {
              "path": "microintents1.json",
              "fallback_threshold": {
                "microintent_name1": 0.3,
                "microintent_name2": 0.5
                "default": 0.5
              }
            }
          ],
          "intents": [
            {
              "intent": "get_weather",
              "nlu": {
                "path": "intents/get_weather.nlu"
              },
              "dm": {
                "path": "intents/get_weather.js"
              },
              "nlg": {
                "path": "intents/get_weather.nlg"
              }
            }
          ]
        }
        """
        name = obj['name']
        if base_name:
            name = '{0}.{1}'.format(base_name, name)

        intents = []
        entities = []

        for intent_data in obj.get('intents', []):
            intent = Intent.from_dict(intent_data, base_name=name)
            intents.append(intent)
            logger.debug('Added intent %s', intent.name)

        for microintent_data in obj.get('microintents', []):
            file_name = microintent_data.get('name')
            if file_name:
                file_name = '{0}.{1}'.format(name, file_name)
            else:
                file_name = name
            microintents = MicrointentStorage.from_dict(
                microintent_data,
                base_name=file_name,
                nlg_checks=nlg_checks
            )
            for microintent in microintents.iter():
                intents.append(microintent)
                logger.debug('Added micro-intent %s', microintent.name)

        for entity_data in obj.get('entities', []):
            entity = Entity.from_dict(entity_data)
            entities.append(entity)
            logger.debug('Added entity %s', entity.name)

        return cls(name=name, intents=intents, entities=entities)

    def __init__(self, name, intents=None, entities=None):
        self.name = name
        self._intents = intents or []
        self._entities = entities or []

    @property
    def intents(self):
        return self._intents

    @property
    def entities(self):
        return self._entities


class AppConfig(object):
    _DEFAULT_NLU_TEMPLATES = {
        'address_ru': 'vins_core/nlu/data/address_ru.txt',
        'city_ru': 'vins_core/nlu/data/city_ru.txt',
        'country': 'vins_core/nlu/data/country.txt',
        'region_ru': 'vins_core/nlu/data/region_ru.txt',
        'poi_ru': 'vins_core/nlu/data/poi_ru.txt',
        'poi_activity_ru': 'vins_core/nlu/data/poi_activity_ru.txt',
        'yandex_search_query': 'vins_core/nlu/data/yandex_search_query.txt',
        'first_name_ru': 'vins_core/nlu/data/first_name_ru.txt',
        'middle_name_ru': 'vins_core/nlu/data/middle_name_ru.txt',
        'last_name_ru': 'vins_core/nlu/data/last_name_ru.txt',
        'date_ru': 'vins_core/nlu/data/date_ru.txt',
        'day_of_week_ru': 'vins_core/nlu/data/day_of_week_ru.txt',
        'abs_time_ru': 'vins_core/nlu/data/abs_time_ru.txt',
        'currency': 'vins_core/nlu/data/currency.txt',
        'rel_time_ru': 'vins_core/nlu/data/rel_time_ru.txt',
    }

    def __init__(self, projects=None):
        self.nlu_templates = self.default_nlu_templates()
        self._projects = projects or []
        self.vinsfile_path = None
        self.nlu = {}
        self.nlg = {}
        self.samples_extractor = {}
        self.form_filling = {}
        self._custom_rules = []
        self.fst = {}
        self.post_classifier = {}

    @classmethod
    def _validate(cls, vinsfile):
        jsonschema.validate(vinsfile, schemas.app_schema)

    def _parse_vinsfile(self, vinsfile):
        self._validate(vinsfile)

    def _parse_projects(self, project_data, parent_project=None, nlg_checks=None):
        base_name = parent_project and parent_project.name
        project = Project.from_dict(
            project_data,
            base_name=base_name,
            nlu_templates=self.nlu_templates,
            nlg_checks=nlg_checks
        )
        self._projects.append(project)
        for entities in project.entities:
            if entities.use_as_template:
                self.nlu_templates['ce_{}'.format(entities.name)] = TemplateEntitiesFormat(
                    entities.name,
                    entities=entities,
                    is_custom_entities=True
                )

        logger.debug('Added project %s', project.name)
        for include in project_data.get('includes', []):
            if include['type'] == 'file':
                include_path = os.path.join(include['path'])
                logger.debug('Include project from file %s', include_path)
                self._parse_projects(
                    load_data_from_file(include_path),
                    parent_project=project, nlg_checks=nlg_checks)

    def parse_vinsfile(self, vinsfile_path, nlg_checks=None):
        self.vinsfile_path = vinsfile_path
        try:
            vinsfile = load_data_from_file(vinsfile_path)
        except Exception as e:
            logger.error('Parse json failed with error: %s', e, exc_info=True)
            raise ValueError('Invalid json at file %s' % vinsfile_path)

        self._parse_vinsfile(vinsfile)

        self.nlu = vinsfile.get('nlu') or {}
        custom_templates = self.nlu.get('custom_templates', {})
        for template in custom_templates:
            self.nlu_templates[template] = TemplateEntitiesFormat(
                template,
                custom_templates[template]
            )

        self.nlg = vinsfile.get('nlg') or {}

        if 'project' in vinsfile:
            self._parse_projects(vinsfile['project'], nlg_checks=nlg_checks)

        self.samples_extractor = self.nlu.get('samples_extractor') or {}
        if self.nlu and 'transition_model' in self.nlu:
            self._custom_rules = self.nlu.get('transition_model').get('custom_rules', [])

        self.fst = vinsfile.get('fst') or {}
        self.form_filling = vinsfile.get('form_filling') or {}
        self.post_classifier = vinsfile.get('post_classifier') or {}

    @classmethod
    def default_nlu_templates(cls):
        return {name: TemplateEntitiesFormat(name, value)
                for name, value in cls._DEFAULT_NLU_TEMPLATES.iteritems()}

    @property
    def intents(self):
        for p in self._projects:
            for i in p.intents:
                yield i

    @property
    def entities(self):
        for p in self._projects:
            for e in p.entities:
                yield e

    @property
    def nlg_templates(self):
        for i in self.intents:
            if i.nlg:
                yield i.nlg

    @property
    def dm_forms(self):
        for i in self.intents:
            if i.dm:
                yield i.dm

    @property
    def projects(self):
        return self._projects

    @property
    def custom_rules(self):
        return self._custom_rules

    @staticmethod
    def from_vinsfile(vinsfile_path):
        app_conf = AppConfig()
        app_conf.parse_vinsfile(vinsfile_path)

        return app_conf


def get_archive_from_cfg(model):
    model_path = get_resource_full_path(model['path'])
    logger.info('Loading model from %s', model_path)
    archiver = getattr(archives, model['archive'])
    return archiver(
        path=model_path, sha256=model.get('sha256')
    )


def load_app_config(app_name):
    # type: (AnyStr) -> AppConfig

    vinsfile = os.path.join(app_name, 'config/Vinsfile.json')
    app_conf = AppConfig()
    app_conf.parse_vinsfile(vinsfile)

    return app_conf


def add_checks_to_microintents_schema(schema, checks):
    if checks is None:
        return schema
    schema = copy.deepcopy(schema)
    branched_nlg = schema['properties']['nlg']['oneOf'][-1]
    branched_nlg['properties'] = {
        check: {'type': 'array', 'items': {'type': 'string'}}
        for check in (checks.keys() + ['else'])
    }
    branched_nlg['additionalProperties'] = False
    return schema
