# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

from vins_core.nlu.anaphora.features.base import BaseFeatureExtractor
from vins_core.nlu.anaphora.mention import Mention

logger = logging.getLogger(__name__)


class BasicFeatureExtractor(BaseFeatureExtractor):
    def extract_single_mention_features(self, anaphor, antecedent, features_dict):
        anaphor_proform = (anaphor.type == Mention.PRONOUN_TYPE)
        anaphor_adverb = (anaphor.pos == 'ADVPRO')

        antecedent_noun = (antecedent.type == Mention.NOUN_TYPE)
        antecedent_noun_phrase = (antecedent.type == Mention.NOUN_PHRASE_TYPE)

        cond = True

        if anaphor_adverb:
            cond = (antecedent_noun or antecedent_noun_phrase) and antecedent.geo

        cond &= (anaphor_proform and (antecedent_noun or antecedent_noun_phrase) and
                 (not anaphor.sg_pl or anaphor.sg_pl == antecedent.sg_pl) and
                 (not anaphor.anim or not antecedent.anim or (anaphor.anim == antecedent.anim)) and
                 (not anaphor.gender or not antecedent.gender or (anaphor.gender in antecedent.gender)))
        features_dict['advpro'] = int(anaphor_adverb)  # anaphor is pronominal adverb
        features_dict['antecedent_type'] = antecedent.type  # noun or noun phrase or pronoun
        features_dict['antecedent_geo'] = int(antecedent.geo)  # antecedent is geo entity
        features_dict['same_number'] = int(not anaphor.sg_pl or anaphor.sg_pl == antecedent.sg_pl)
        features_dict['antecedent_number'] = str(antecedent.sg_pl)  # single, plural or None
        features_dict['same_animate'] = int(not anaphor.anim or not antecedent.anim or
                                            (anaphor.anim == antecedent.anim))
        features_dict['same_gender'] = int(not anaphor.gender or not antecedent.gender or
                                           (anaphor.gender in antecedent.gender))
        features_dict['geo_advpro'] = int(anaphor_adverb and antecedent.geo)  # match geo entity with pronominal adverb
        features_dict['ant_animate'] = int(antecedent.anim == 'anim')
        features_dict['antecedent_source'] = antecedent.source  # syntax or entitysearch
        features_dict['antecedent_word_cnt'] = len(antecedent.text.split())  # length on antecedent text in words
        features_dict['is_rule_based_condition'] = int(cond)

    def add_features(self, context, features):
        for phrase_mentions, phrase_features in zip(context.antecedents, features):
            for antecedent, mention_features in zip(phrase_mentions, phrase_features):
                self.extract_single_mention_features(context.anaphor, antecedent, mention_features)
