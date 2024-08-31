# coding: utf-8

import itertools
import logging
import re

from nlu_service import named_entity
from nlu_service.services import lemmer_wrapper


logger = logging.getLogger(__name__)

RE_REMOVE_DIGITS = re.compile(r'\d')


def entity_is_not_within_known_entities(start_token, end_token, known_entity_bounds):
    return all(
        end_token <= known_start or start_token >= known_end
        for known_start, known_end in known_entity_bounds
    )


class WizardParser(object):

    def _extract_numbers(self, wizard_markup):
        for number in wizard_markup.get('Numbers', []):
            token_start = number['Tokens']['Begin']
            token_end = number['Tokens']['End']
            yield named_entity.NumberEntity(
                token_start,
                token_end,
                number['Value'],
            )

    @staticmethod
    def _is_valid_name_token(morph, marked_tokens):
        return (
            'Lemmas' in morph
            and morph['Tokens']['Begin'] not in marked_tokens
            and 'Text' in morph['Lemmas'][0]
            and lemmer_wrapper.is_valid_word(morph['Lemmas'][0]['Text'])
        )

    def _get_unmatched_name_tokens(self, morph_markup, marked_tokens):
        for morph in filter(lambda m: self._is_valid_name_token(m, marked_tokens), morph_markup):
            token_position = morph['Tokens']['Begin']
            lemma = morph['Lemmas'][0]
            grammem_sets = lemma.get('Grammems', [])
            if any('persn' in grammem_set for grammem_set in grammem_sets):
                yield 'first_name', token_position, lemma
            elif any('patrn' in grammem_set for grammem_set in grammem_sets):
                yield 'patronymic_name', token_position, lemma
            elif any('famn' in grammem_set for grammem_set in grammem_sets):
                yield 'last_name', token_position, lemma

    def _extract_names_from_grammems(self, wizard_markup, known_fio_tokens):
        # wizard Fio rule will work only if last name was specified
        # to match first and patronymic names we will process Morph rule output looking for "persn" and "patrn" grammems
        # that weren't previously matched
        name = {}
        last_position = None  # not used
        for kwarg, position, lemma in self._get_unmatched_name_tokens(
            wizard_markup.get('Morph', []),
            known_fio_tokens,
        ):
            if kwarg in name:
                yield named_entity.FioEntity(**name)
                name = {}
                last_position = None
            if kwarg not in name and (last_position is None or position - last_position == 1):
                name.setdefault('start_token', position)
                name['end_token'] = position + 1
                name[kwarg] = lemma['Text']
                last_position = position
            elif name:
                yield named_entity.FioEntity(**name)
        if name:
            yield named_entity.FioEntity(**name)

    def _extract_names(self, wizard_markup):
        marked_tokens = set()
        for fio in wizard_markup.get('Fio', []):
            start_token = fio['Tokens']['Begin']
            end_token = fio['Tokens']['End']
            yield named_entity.FioEntity(
                start_token,
                end_token,
                first_name=fio.get('FirstName'),
                last_name=fio.get('LastName'),
                patronymic_name=fio.get('Patronymic'),
            )
            marked_tokens.update(xrange(start_token, end_token))
        for fio in self._extract_names_from_grammems(wizard_markup, marked_tokens):
            yield fio

    def _extract_geo_entities(self, wizard_markup):
        known_geo_entity_bounds = []
        for geoaddr in wizard_markup.get('GeoAddr', []):
            start_token = geoaddr['Tokens']['Begin']
            end_token = geoaddr['Tokens']['End']
            if entity_is_not_within_known_entities(
                start_token,
                end_token,
                known_geo_entity_bounds,
            ):
                geo_entity = named_entity.GeoEntity.from_wizard_geoaddr(geoaddr)
                if geo_entity:
                    known_geo_entity_bounds.append((start_token, end_token))
                    yield geo_entity

    def _extract_date_entities(self, wizard_markup):
        for date in wizard_markup.get('Date', []):
            yield named_entity.DateTimeEntity.from_wizard_date(date)

    def extract_entities(self, wizard_markup):
        return list(itertools.chain(
            self._extract_numbers(wizard_markup),
            self._extract_geo_entities(wizard_markup),
            self._extract_date_entities(wizard_markup),
            self._extract_names(wizard_markup),
        ))

    def extract_tokens(cls, wizard_markup):
        # adopted from vins_core.nlu.syntax.Token.extract_tokens
        tokens = []
        wiz_morph = wizard_markup.get('Morph', [])
        wiz_tokens = wizard_markup.get('Tokens', [])
        for m, t in zip(wiz_morph, wiz_tokens):
            if 'Lemmas' in m:
                lemma = m['Lemmas'][0]
                if 'Text' in lemma:
                    tokens.append(t['Text'])
        return tokens
