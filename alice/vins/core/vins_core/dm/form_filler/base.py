# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from operator import attrgetter

from vins_core.config.app_config import get_archive_from_cfg
from vins_core.ner.fst_presets import FstParserFactory
from vins_core.nlg.template_nlg import TemplateNLG
from vins_core.nlu.custom_entities_tools import load_mixed_parsers
from vins_core.nlu.flow_nlu import FlowNLU
from vins_core.nlu.flow_nlu_factory.transition_model import create_transition_model
from vins_core.config.intent_config_loaders import parse_intent_configs, iterate_nlu_sources_for_intent
from vins_core.utils.config import get_setting
from vins_core.dm.response import ClientActionDirective, ServerActionDirective

logger = logging.getLogger(__name__)


class BaseDialogManager(object):

    """
    Base class which represents business logic of vins.
    It implements constructor methods (``from_config()`` and ``__init__()`` and has abstract method
    ``handle()`` which must be implemented in child class.
    ``handle()`` is responsible for processing user's utterances, managing nlu/nlg etc, keeping sessions etc.
    """

    def __init__(self, intents, intent_to_form, nlu, nlg):
        """
        Args:
            intents: A list of Intent instances for this dialog.
            intent_to_form (dict): Mapping intent_name->``Form``. Form represents dialog state scheme for each intent.
            nlu (vins_core.nlu.base_nlu.BaseNLU): NLU module which is used for utterance classification
                and slot tagging.
            nlg (vins_core.nlg.template_nlg.NLG): NLG module which is used for generating responses based
                on form.
        """
        self._nlu = nlu
        self._nlg = nlg
        self._intent_to_form = intent_to_form.copy()
        self._name_to_intent = {intent.name: intent for intent in intents}

    @property
    def nlu(self):
        return self._nlu

    @property
    def nlg(self):
        return self._nlg

    @property
    def samples_extractor(self):
        if self._nlu:
            return self._nlu.samples_extractor
        else:
            return None

    def has_intent(self, intent_name):
        return intent_name in self._name_to_intent

    def get_intent(self, intent_name):
        return self._name_to_intent[intent_name]

    @classmethod
    def from_config(
            cls, app_config,
            load_data=False,
            load_model=True,
            **kwargs
    ):
        logger.debug('BaseDialogManager.from_config(...) started...')

        return cls(**cls._constructor_kwargs_from_config(
            app_config, load_data, load_model, **kwargs))

    @classmethod
    def _constructor_kwargs_from_config(cls, app_config, load_data, load_model, **kwargs):
        constructor_kwargs = {}

        # - initialize nlg -
        constructor_kwargs['nlg'] = cls._create_nlg(app_config)

        # - initialize nlu -
        constructor_kwargs['nlu'], constructor_kwargs['intents'] = cls._create_nlu(
            app_config,
            load_data=load_data, load_model=load_model,
            **kwargs
        )

        # - initialize intent2form_map -
        constructor_kwargs['intent_to_form'] = cls._create_intent_to_form_map(app_config)

        return constructor_kwargs

    @classmethod
    def _create_nlu(
        cls, app_cfg, load_data, load_model, **kwargs
    ):
        exact_matched_intents = set(app_cfg.nlu.get('exact_matching', {}).get('intents', []))
        env_exact_matched_intents = get_setting('EXACT_MATCHED_INTENTS', '')

        if env_exact_matched_intents:
            exact_matched_intents.update(env_exact_matched_intents.split(','))

        default_fallback_threshold = app_cfg.nlu.get('fallback_threshold')

        fst_parser_factory = cls._create_fst_parser_factory(app_cfg.nlu.get('fst'))
        fst_parsers = app_cfg.nlu.get('fst', {}).get('parsers', [])

        nlu_cfg = dict(
            default_fallback_threshold=default_fallback_threshold,
            exact_matched_intents=exact_matched_intents,
            anaphora_config=app_cfg.nlu.get('anaphora_resolution', {}),
            fst_parser_factory=fst_parser_factory,
            fst_parsers=fst_parsers,
            request_filters_dict=kwargs.get('request_filters_dict')
        )
        nlu_cfg.update(app_cfg.nlu)
        nlu = FlowNLU(**nlu_cfg)

        intent_configs = parse_intent_configs(app_cfg.intents)
        intents = map(attrgetter('intent'), intent_configs)

        cls._load_intents(nlu, app_cfg, intent_configs, load_data)

        if load_data:
            logger.info(
                'Intents / items distribution:\n%s',
                '\n'.join('{} {}'.format(intent, len(items)) for intent, items in nlu.nlu_sources_data.iteritems())
            )

        if load_model and app_cfg.nlu.get('compiled_model'):
            with get_archive_from_cfg(
                app_cfg.nlu['compiled_model']
            ) as arch:
                cls._load_custom_entities(app_cfg, nlu, arch)
                logger.info("Start NLU model loading...")
                nlu.load(arch)
                logger.info("NLU model have been loaded")
        else:
            nlu.load()

        cls._create_nlu_add_transition_model_to_nlu(app_cfg, nlu, intents)
        nlu.validate()

        return nlu, intents

    @classmethod
    def _create_fst_parser_factory(cls, cfg):
        factory = FstParserFactory.from_config(cfg)
        logger.info('Loading fst parsers')
        factory.load()
        return factory

    @classmethod
    def _load_custom_entities(cls, app_cfg, nlu, archive):
        if not archive:
            return
        mixed_parsers = load_mixed_parsers(app_cfg, archive)
        nlu.set_entity_parsers(mixed_parsers)

    @staticmethod
    def _load_intents(nlu, app_cfg, intent_configs, load_data):
        for intent_config in intent_configs:
            tagger_data_keys = set()

            if intent_config.nlu:
                for nlu_source in iterate_nlu_sources_for_intent(intent_config, app_cfg.nlu_templates):
                    nlu.add_input_data(intent_config.name, nlu_source)

                if load_data and intent_config.nlu.config:
                    nlu.nlu_sources_data.load(intent_config.name)
                    # prevent training / loading any tagger if there is nothing to tag
                    if any(len(nlu_item.slots) for nlu_item in nlu.nlu_sources_data[intent_config.name]):
                        tagger_data_keys.update(intent_config.tagger_data_keys)

            nlu.add_intent(
                intent_name=intent_config.name,
                prior=intent_config.prior,
                fallback_threshold=intent_config.fallback_threshold,
                tagger_data_keys=tagger_data_keys,
                trainable_classifiers=intent_config.trainable_classifiers,
                fallback=intent_config.fallback,
                total_fallback=intent_config.total_fallback,
                positive_sampling=intent_config.positive_sampling,
                negative_sampling_from=intent_config.negative_sampling_from,
                negative_sampling_for=intent_config.negative_sampling_for,
                scenarios=intent_config.scenarios,
                utterance_tagger=intent_config.utterance_tagger
            )

    @staticmethod
    def _create_nlu_add_transition_model_to_nlu(app_cfg, nlu, intents):
        logger.info('Start transition model creating')
        transition_model_cfg = app_cfg.nlu.get('transition_model', dict())
        transition_model = create_transition_model(
            intents=intents, **transition_model_cfg)
        nlu.set_transition_model(transition_model)
        logger.info('Transition model has been created')

    @staticmethod
    def _create_nlg(app_cfg):
        logger.info('Compiling NLG templates')
        includes = app_cfg.nlg.get('includes', [])
        global_nlg = app_cfg.nlg.get('global', [])
        nlg = TemplateNLG(
            global_templates=global_nlg,
            templates_dir=includes,
            add_globals={
                'client_action_directive': ClientActionDirective.create_dict,
                'server_action_directive': ServerActionDirective.create_dict,
            },

        )

        for intent in app_cfg.intents:
            if intent.nlg or intent.nlg_filename:
                nlg.add_intent(intent.name, intent.nlg, intent.nlg_filename)
        logger.info('NLG templates have been compiled.')
        return nlg

    @staticmethod
    def _create_intent_to_form_map(app_cfg):
        intent_to_form = dict()
        for intent in app_cfg.intents:
            form = intent.dm
            if not form:
                try:
                    form = next(i for i in app_cfg.intents if i.name == intent.parent).dm
                except StopIteration:
                    logger.error("No parental intent found for '%s'", intent.name, exc_info=True)
                    raise
            intent_to_form[intent.name] = form
        return intent_to_form
