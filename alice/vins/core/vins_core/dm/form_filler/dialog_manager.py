# vim: set fileencoding=utf-8
from __future__ import unicode_literals

import attr
import copy
import logging
from itertools import izip

from vins_core.dm.form_filler.nlu_postprocessor import (
    DatetimeNowNluPostProcessor, SortNluPostProcessor,
    RescoringNluPostProcessor, CutoffNluPostProcessor,
    NormalizingNluPostProcessor, ContinuedSlotNluPostProcessor,
    ChainedNluPostProcessor, FirstFrameChoosingNluPostProcessor,
    IntentGroupNluPostProcessor
)
from vins_core.dm.frame_scoring.slot_type import SlotTypeFrameScoring, ActiveSlotFrameScoring, DisabledSlotFrameScoring
from vins_core.dm.frame_scoring.multislot_scoring import MultislotFrameScoring
from vins_core.dm.frame_scoring.tagger_score import TaggerScoreFrameScoring
from vins_core.dm.form_filler.base import BaseDialogManager
from vins_core.dm.form_filler.form_candidate import FormCandidate
from vins_core.dm.form_filler.feature_updater import create_feature_updater
from vins_core.nlu.base_nlu import NLUHandleResult
from vins_core.common.slots_map_utils import matched
from vins_core.utils.decorators import time_info
from vins_core.utils.metrics import sensors
from vins_core.utils.intent_renamer import get_intent_for_metrics
from vins_core.nlu.nlu_utils import semantic_frames_to_taggers_scores
from vins_core.nlu.features.base import IntentScore
from vins_core.nlu.reranker.reranker import create_reranker, create_default_reranker
from vins_core.nlu.reranker.factor_calcer import FactorCalcer
from vins_core.logger.utils import dump_object_sequence

logger = logging.getLogger(__name__)


@attr.s
class IntentChangeInfo(object):
    session_starts = attr.ib()
    intent_changed = attr.ib()
    context_changed = attr.ib()
    reset_form = attr.ib()


class FormFillingDialogManager(BaseDialogManager):
    """
    Core class which represents business logic of vins. It handles user's utterances (``handle()`` method),
    manages nlu/nlg etc, keeps sessions etc.

    Instantiate it using classmethod ``DialogManager.from_config(app_config)`` or with constructor directly.
    """

    @classmethod
    def _constructor_kwargs_from_config(cls, app_config, load_data, load_model, **kwargs):
        constructor_kwargs = super(FormFillingDialogManager, cls)._constructor_kwargs_from_config(
            app_config, load_data, load_model, **kwargs
        )
        constructor_kwargs['max_intents'] = app_config.form_filling.get('max_intents', 1)
        constructor_kwargs['feature_updater'] = kwargs.get(
            'feature_updater',
            app_config.form_filling.get('feature_updater')
        )
        constructor_kwargs['reranker_config'] = app_config.post_classifier.get('reranker')
        constructor_kwargs['factor_calcer_config'] = app_config.post_classifier.get('factor_calcer', {})
        return constructor_kwargs

    def __init__(
        self, intents,
        intent_to_form,
        nlu,
        nlg,
        nlu_post_processor=None,
        max_intents=1,
        feature_updater=None,
        reranker_config=None,
        factor_calcer_config=None
    ):
        """
        Args:
            intents: A list of Intent instances for this dialog.
            intent_to_form (dict): Mapping intent_name->``Form``. Form represents dialog state scheme for each intent.
            nlu (vins_core.nlu.base_nlu.BaseNLU): NLU module which is used for utterance classification
                and slot tagging.
            nlg (vins_core.nlg.template_nlg.TemplateNLG): NLG module which is used for generating responses based
                on form.
        """
        super(FormFillingDialogManager, self).__init__(intents, intent_to_form, nlu, nlg)
        self._max_intents = max_intents

        # TODO: make this configurable
        if nlu_post_processor is None:
            self._nlu_post_processor = IntentGroupNluPostProcessor(
                ChainedNluPostProcessor([
                    ContinuedSlotNluPostProcessor(),
                    RescoringNluPostProcessor([
                        SlotTypeFrameScoring(),
                        ActiveSlotFrameScoring(boosting_score=10),
                        TaggerScoreFrameScoring(),
                        MultislotFrameScoring(forms_map=intent_to_form),
                        DisabledSlotFrameScoring(0)
                    ]),
                    CutoffNluPostProcessor(cutoff=0, dont_kill_all_frames=True),
                    SortNluPostProcessor(),
                    DatetimeNowNluPostProcessor(),
                    NormalizingNluPostProcessor(),
                    FirstFrameChoosingNluPostProcessor()
                ])
            )
        else:
            self._nlu_post_processor = nlu_post_processor

        self._feature_updater = create_feature_updater(feature_updater)

        factor_calcer = FactorCalcer.from_config(factor_calcer_config or {})
        if reranker_config:
            self._reranker = create_reranker(reranker_config['model'], reranker_config.get('params', {}), factor_calcer)
        else:
            self._reranker = create_default_reranker()

    def _copy_shared_values_from(self, form, other_form, sample, session, app, req_info, response, **kwargs):
        if not other_form:
            return

        logger.debug('Importing values from the previous form')

        for slot in form.slots:
            if not slot.import_tags:
                continue

            for other_slot in other_form.slots:
                if not other_slot.share_tags:
                    continue

                intersection = slot.import_tags.intersection(other_slot.share_tags)
                if intersection:
                    form.shares_slots_with_previous_form = True

                if other_slot.value is not None and intersection:
                    # Copy from the first suitable slot
                    # TODO: do some type compatibility check here?
                    self._fill_slot(
                        slot, other_slot.value, other_slot.source_text, other_slot.value_type,
                        form, sample, session, app, req_info=req_info,
                        event='import', response=response, **kwargs
                    )
                    break

    @staticmethod
    def _get_slot_data(value, slot):
        entity = next(matched(value, slot.types, slot.matching_type), None)
        source_text = value.get('substr')
        slot_value = entity.value if entity else None
        value_type = entity.type.lower() if entity else None
        return slot_value, source_text, value_type

    def _fill_form(self, form, frame, sample, session, app, req_info, response, **kwargs):
        if self.samples_extractor and 'entitysearch' in self.samples_extractor.pipeline:
            self._fill_slots_with_entity_search(form, frame, sample, session, app, req_info, response, **kwargs)
        form.raise_event(
            'prepare_form_update', dm=self, sample=sample, sample_features=None, session=session, app=app,
            req_info=req_info, required_handler=False, response=response, frame=frame, **kwargs)
        self._fill_slots_from_annotations(form, sample, session, app, req_info, response, **kwargs)

        for slot_name, value in frame['slots'].iteritems():
            slot = form.get_slot_by_name(slot_name)
            slot_value, source_text, value_type = map(list, izip(*(self._get_slot_data(part, slot) for part in value)))
            # if the slot has a single value, move it out of the list
            if len(slot_value) == 1:
                slot_value, source_text, value_type = slot_value[0], source_text[0], value_type[0]
            self._fill_slot(
                slot,
                value=slot_value, source_text=source_text,
                value_type=value_type,
                form=form, sample=sample, session=session, app=app, req_info=req_info,
                response=response, **kwargs
            )

    def _fill_slots_from_annotations(self, form, sample, session, app, req_info, response, **kwargs):
        for slot in form.slots:
            if not slot.source_annotation:
                continue
            source_annotation = sample.annotations.get(slot.source_annotation)
            if not source_annotation:
                continue
            if source_annotation.value_type not in slot.types:
                logger.warning('Wrong value type of annotation %s. Can not fill slot %s.'
                               'Found: %s. Expected: %s' % (slot.source_annotation, slot.name,
                                                            source_annotation.value_type, ', '.join(slot.types)))
                continue
            self._fill_slot(
                slot,
                value=source_annotation.value, source_text=sample.text,
                value_type=source_annotation.value_type,
                form=form, sample=sample, session=session, app=app, req_info=req_info,
                response=response, **kwargs
            )

    def _fill_slots_with_entity_search(self, form, frame, sample, session, app, req_info, response, **kwargs):
        for slot in form.slots:
            is_slot_in_frame = slot.name in frame['slots']
            is_entity_import_supported = (slot.import_entity_types or slot.import_entity_tags)
            try_entity_search_import = (not is_slot_in_frame and is_entity_import_supported)

            if not try_entity_search_import:
                continue

            logger.debug("Trying to fill slot '%s' using EntitySearchAnnotation.", slot.name)

            suitable_entity = self.nlu.anaphora_resolver.find_suitable_anaphoric_entity(slot, sample, session)

            if suitable_entity:
                logger.debug('Found entity "%s", importing it to slot "%s"', suitable_entity.to_dict(), slot.name)
                self._fill_slot(
                    slot, value=suitable_entity.name, source_text=suitable_entity.name, value_type='string',
                    form=form, sample=sample, session=session, app=app, req_info=req_info, response=response, **kwargs
                )

    def _fill_slot(self, slot, value, source_text, value_type, form,
                   sample, session, app, req_info, response, event='fill', **kwargs):
        slot.set_value(copy.copy(value), value_type, source_text)

        # Check if this is slot from group
        for slot_group in form.required_slot_groups:
            if slot.name in slot_group.slots:
                for slot_from_group in slot_group.slots:
                    form.get_slot_by_name(slot_from_group).active = False
                # DIALOG-3802
                form.get_slot_by_name(slot_group.slot_to_ask).optional = True

        slot.active = False
        slot.raise_event(
            event,
            dm=self, sample=sample, session=session,
            app=app, form=form, req_info=req_info, required_handler=False,
            response=response, **kwargs
        )
        logger.debug('Slot %s set to %s', slot.name, value)

    def _ask_slot(self, slot, form, sample, session, app, req_info, response, **kwargs):
        ask_event = 'ask'
        if slot.active:
            # This slot has been asked before. If there is a reask handler, use it.
            if slot.get_event_handlers('reask'):
                ask_event = 'reask'
            else:
                logger.debug(
                    "No 'reask' event handlers specified for the slot '%s.%s', falling back to 'ask'." % (
                        form.name, slot.name
                    )
                )

        # Check if this is ask_slot from group
        for slot_group in form.required_slot_groups:
            if slot_group.slot_to_ask == slot.name:
                for slot_from_group in slot_group.slots:
                    form.get_slot_by_name(slot_from_group).active = True

        slot.active = True
        slot.raise_event(
            ask_event, dm=self, sample=sample, session=session,
            app=app, form=form, req_info=req_info, required_handler=True,
            response=response, **kwargs
        )

    def _submit_form(self, form, sample, sample_features, session, app, req_info, response, **kwargs):
        form.raise_event(
            'submit', dm=self, sample=sample, sample_features=sample_features, session=session,
            app=app, req_info=req_info, required_handler=True, response=response, **kwargs)

    @sensors.with_timer('dm_handle_form_time')
    def handle_form(self, sample, sample_features, session, app, req_info, response, precomputed_data={}, **kwargs):
        form = session.form

        # Check if there are required slots that need to be filled
        empty_required_slots = [s for s in form.slots if not s.optional and s.value is None]
        for slot in empty_required_slots:
            self._ask_slot(slot, form, sample, session, app, req_info, response=response, **kwargs)
            return

        # The form is ready for submitting
        logger.debug('All required slots and slot groups have been filled, submitting form')

        self._submit_form(
            form, sample, sample_features, session, app, req_info,
            response=response, precomputed_data=precomputed_data, **kwargs
        )

    @time_info('dialog_manager_get_semantic_frames')
    def get_semantic_frames(self, sample, session, req_info, max_intents=1, only_sample_features=False, **kwargs):
        nlu_result = self.nlu.handle(
            sample,
            req_info=req_info,
            session=session,
            max_intents=max_intents,
            only_sample_features=only_sample_features,
            **kwargs
        )

        if nlu_result.sample_features is not None:
            nlu_result.sample_features.add_tagger_scores(
                'nlu_handle',
                semantic_frames_to_taggers_scores(nlu_result.semantic_frames)
            )

        if only_sample_features:
            return nlu_result

        with sensors.timer('dm_postprocessing_time'):
            post_processed_frames = self._nlu_post_processor(
                nlu_result.semantic_frames,
                session,
                sample=sample,
                client_time=req_info.client_time,
                forms_map=self._intent_to_form
            )
        nlu_result.semantic_frames = post_processed_frames

        if nlu_result.sample_features is not None:
            nlu_result.sample_features.add_tagger_scores(
                'post_processed',
                semantic_frames_to_taggers_scores(nlu_result.semantic_frames)
            )
            intent_scores = [IntentScore(name=frame['intent_name'], score=frame['confidence'])
                             for frame in nlu_result.semantic_frames]
            nlu_result.sample_features.add_classification_scores('nlu_post_processor', intent_scores)

        return nlu_result

    def _prepare_session(self, session):
        if session and session.form:
            for active_slot in session.form.active_slots():
                if active_slot.expected_values:
                    active_slot.expected_values = self.samples_extractor(active_slot.expected_values)

    def new_form(
        self, intent_name, response=None, app=None,
        previous_form=None, sample=None, session=None, req_info=None,
        **kwargs
    ):
        """Create new form for specified intent using intent2form map. Optionally, previous_form can be passed and its
            shared values will be incorporated in new form.

        Args:
            intent_name (str or unicode): The name of the intent associated with the new form.
            previous_form (Form): Previous form which shared values will be copied.

        Returns:
            form (Form): A new form.
        """
        form = copy.deepcopy(self._intent_to_form[intent_name])
        if previous_form:
            self._copy_shared_values_from(
                form, previous_form, sample, session=session, req_info=req_info,
                response=response,
                app=app,
                **kwargs
            )

        return form

    def _get_intent_change_info(self, session, current_intent_name):
        current_intent = self.get_intent(current_intent_name)
        previous_intent_name = session.intent_name
        session_starts = session.intent_name is None
        intent_changed = not session_starts and previous_intent_name != current_intent.name
        context_changed = intent_changed and current_intent.parent_name != previous_intent_name
        reset_form = current_intent.reset_form
        return IntentChangeInfo(
            session_starts=session_starts,
            intent_changed=intent_changed,
            context_changed=context_changed,
            reset_form=reset_form
        )

    def _apply_frame_to_form(self, app, response, sample, session, req_info, frame, **kwargs):
        logger.debug('Semantic frame: %s', frame)

        form = copy.deepcopy(session.form)
        intent_name = frame['intent_name']
        info = self._get_intent_change_info(session, intent_name)

        if not form or info.session_starts or info.context_changed or info.reset_form:
            form = self.new_form(
                intent_name=intent_name,
                session=session,
                previous_form=form,
                sample=sample,
                app=app,
                response=response,
                req_info=req_info,
                **kwargs
            )
        else:
            form.shares_slots_with_previous_form = len(form.slots) > 0

        # If we are trying to change form to prohibited intent we should not fill slots from frame
        if self.can_fill_form(req_info, form, intent_name):
            self._fill_form(
                form, frame, sample, session=session, req_info=req_info,
                response=response, app=app, **kwargs
            )

        return form

    @time_info('FormFillingDialogManager.handle')
    @sensors.with_timer('dm_handle_time')
    def handle(self, req_info, session, **kwargs):
        self._prepare_session(session)
        with sensors.timer('dm_samples_extractor_time'):
            sample = self.samples_extractor(
                [req_info.utterance],
                session,
                filter_errors=True,
                app_id=req_info.app_info.app_id or '',
                request_id=req_info.request_id,
                features=req_info.features,
                not_use_wizard_from_mm_flag=req_info.experiments['not_use_wizard_from_megamind'],
                not_use_entity_from_mm_flag=req_info.experiments['not_use_entity_from_megamind']
            )[0]
        return self.handle_with_extracted_samples(sample, req_info, session, **kwargs)

    @time_info('FormFillingDialogManager.handle_with_extracted_samples')
    def handle_with_extracted_samples(self, sample, req_info, session, app, response,
                                      only_sample_features=False, **kwargs):
        form_candidates = []

        # TODO: replace dict-based frames by objects
        nlu_result = self.get_semantic_frames(
            sample, session, req_info, max_intents=self._max_intents, only_sample_features=only_sample_features
        )
        if only_sample_features:
            return nlu_result

        sample_features = nlu_result.sample_features
        for index, frame in enumerate(nlu_result.semantic_frames):
            form = self._apply_frame_to_form(app, response, sample, session, req_info, frame, **kwargs)
            form_candidates.append(FormCandidate(form=form,
                                                 intent=frame.get('intent_candidate'),
                                                 frame=frame,
                                                 index=index))

        exact_matched_intent = len(form_candidates) > 0 and form_candidates[0].intent.score >= 1.5

        sample_features, form_candidates = self._update_features(app, sample_features, form_candidates,
                                                                 exact_matched_intent, session, req_info)
        form_candidates = self._remove_fake_candidates(form_candidates)
        form_candidates = self._rerank_form_candidates(form_candidates, sample_features, exact_matched_intent,
                                                       session, req_info)

        assert form_candidates

        best_candidate = form_candidates[0]

        logger.debug('Won intent: %s', best_candidate.intent.name)
        if sample_features is not None:
            intent_scores = [IntentScore(name=best_candidate.intent.name)]
            sample_features.add_classification_scores('form_chooser', intent_scores)

        with sensors.labels_context({'intent_name': get_intent_for_metrics(best_candidate.intent.name)}):
            session.change_form(best_candidate.form)
            intent_change_info = self._get_intent_change_info(session, best_candidate.intent.name)
            if intent_change_info.session_starts or intent_change_info.intent_changed:
                session.change_intent(self.get_intent(best_candidate.intent.name))
            self.handle_form(
                sample,
                sample_features=sample_features,
                session=session,
                req_info=req_info,
                response=response,
                app=app,
                precomputed_data=best_candidate.precomputed_data,
                **kwargs
            )

        semantic_frames = [candidate.frame for candidate in form_candidates]
        return NLUHandleResult(semantic_frames=semantic_frames, sample_features=sample_features)

    def can_fill_form(self, req_info, form, intent_name):
        # in PA app, some intents are prohibited, and for them the form is not filled
        # (see PersonalAssistantDialogManager.new_form)
        return True

    @sensors.with_timer('dm_update_custom_entities_time')
    def update_custom_entities(self, archive):
        self.nlu.update_custom_entities(archive)

    def _update_features(self, app, sample_features, form_candidates, exact_matched_intent, session, req_info):
        if self._feature_updater is not None and not exact_matched_intent:
            feature_updater_result = self._feature_updater.update_features(app, sample_features, session, req_info,
                                                                           form_candidates)
            return feature_updater_result.sample_features, feature_updater_result.form_candidates

        return sample_features, form_candidates

    @staticmethod
    def _remove_fake_candidates(form_candidates):
        return [form_candidate for form_candidate in form_candidates if not form_candidate.intent.is_fake]

    def _rerank_form_candidates(self, form_candidates, sample_features, exact_matched_intent, session, req_info):
        if self._reranker and not exact_matched_intent:
            form_candidates = self._reranker(sample_features, form_candidates, req_info)

            if sample_features is not None:
                intent_scores = [IntentScore(name=form_candidate.intent.name, score=form_candidate.intent.score)
                                 for form_candidate in form_candidates]
                sample_features.add_classification_scores('reranker', intent_scores)

            logger.debug(
                'Candidate intents after reranker:\n%s',
                dump_object_sequence([cand.intent for cand in form_candidates], ['name', 'score'])
            )
        return form_candidates


DialogManager = FormFillingDialogManager
