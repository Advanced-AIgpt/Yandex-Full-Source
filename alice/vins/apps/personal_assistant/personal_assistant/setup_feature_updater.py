# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import attr
import logging
import copy

from itertools import izip
from personal_assistant.api.personal_assistant import PersonalAssistantAPIError
from personal_assistant.setup_features import get_setup_features, has_setup_features
from vins_core.dm.form_filler.feature_updater import FeatureUpdater, FeatureUpdaterResult


logger = logging.getLogger(__name__)


class BassSetupFeatureUpdater(FeatureUpdater):
    NAME = 'bass_setup'

    def update_features(self, app, sample_features, session, req_info, form_candidates):
        if req_info is None or req_info.experiments['bass_setup_features'] is None:
            return FeatureUpdaterResult(sample_features, form_candidates)

        assert len(form_candidates) > 0

        intent_forms = [candidate.form for candidate in form_candidates]
        try:
            setup_result = app.setup_forms(
                forms=intent_forms,
                session=session,
                req_info=req_info,
                sample=sample_features.sample
            )
        except PersonalAssistantAPIError:
            logger.warning('Got PersonalAssistantAPIError', exc_info=True)
            return FeatureUpdaterResult(sample_features, form_candidates)

        if len(setup_result) != len(intent_forms):
            raise RuntimeError('The number of output forms should be equal to the number of input forms. ' +
                               'Input: {0}, Output: {1}'.format(len(setup_result), len(intent_forms)))

        sample_features = copy.deepcopy(sample_features)

        raw_factors_data = {}
        for form_setup in setup_result:
            if form_setup and form_setup.meta and form_setup.meta.factors_data is not None:
                if has_setup_features(form_setup.form.name):
                    feature_extractor = get_setup_features(form_setup.form.name)
                    features = feature_extractor.from_dict(form_setup.meta.factors_data)
                    sample_features.dense[feature_extractor.name()] = features
                    raw_factors_data[feature_extractor.name()] = form_setup.meta.factors_data

        # TODO: Move is_feasible checking to reranker
        if any(result.meta.is_feasible for result in setup_result):
            form_candidates = [
                attr.evolve(form_candidate, precomputed_data=result.precomputed_data)
                for form_candidate, result in izip(form_candidates, setup_result)
                if result.meta.is_feasible
            ]

        return FeatureUpdaterResult(sample_features, form_candidates, raw_factors_data)
