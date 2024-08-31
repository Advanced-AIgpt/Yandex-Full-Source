# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import logging
import numpy as np
import os
import re
import six

from string import punctuation
from six.moves import range, zip

from alice.nlu.py_libs.syntax_parser import Parser
from alice.nlu.py_libs.tokenizer import tokenize, sentenize

from alice.nlu.py_libs.respect_rewriter.rewrite_classifiers import (
    RewritableSentenceClassifier, RewritablePositionsClassifier)

logger = logging.getLogger(__name__)


_PLURALIZER_FIXLIST = {
    'Я и включи и выключу, никаких проблем!': 'Я и включу и выключу, никаких проблем!',
    'Делись, давай!': 'Делитесь, давайте!',
    'Зачем. Ты. Напомнила. Мне. Эту. Песню?': 'Зачем. Вы. Напомнили. Мне. Эту. Песню?',
    'Конечно, а ты что, сомневалась в моих способностях?': 'Конечно, а вы что, сомневались в моих способностях?',
    'Ты что, сомневалась в моих способностях?': 'Вы что, сомневались в моих способностях?',
    'Так здорово, когда тебя кто-то понимает': 'Так здорово, когда тебя кто-то понимает',
    'Ха, жестокая вы!': 'Ха, жестоки вы!',
    'Запутала меня совсем': 'Запутали меня совсем',
    'Заинтриговала. Что?': 'Заинтриговали. Что?',
    'Прикинь, сама в шоке': 'Прикиньте, сама в шоке',
    'Лучше сходи поешь': 'Лучше сходите поешьте',
    'Любил ? А сейчас не любишь ?': 'Любили? А сейчас не любите?',
    'И что он? Узнал?': 'И что он? Узнал?',
}


_SINGULARIZER_FIXLIST = {
    'Нет, давайте вы будете выполнять свои домашние задания самостоятельно':
        'Нет, давай ты будешь выполнять свои домашние задания самостоятельно',
    'Тогда давайте общаться постоянно. Будете рассказывать мне по вечерам, как день прошёл.':
        'Тогда давай общаться постоянно. Будешь рассказывать мне по вечерам, как день прошёл.',
    'Рада за вас. Чем займётесь сегодня?': 'Рада за тебя. Чем займёмся сегодня?',
    'Как узнаете, скажите': 'Как узнаешь, скажи',
    'Может объясните мне, что это значит?': 'Может объяснишь мне, что это значит?',
    'Почему бы вам не сделать домашнее задание самим?': 'Почему бы тебе не сделать домашнее задание самому?',
    'Рада за вас, что делаете?': 'Рада за тебя, что делаешь?',
    'Тогда давайте сменим тему? Что ещё хотите обсудить?': 'Тогда давай сменим тему? Что ещё хочешь обсудить?',
    'Когда узнаете, скажите': 'Когда узнаешь, скажи',
}


_PLURALIZER_POSTPROCESS_FIXLIST = {
    (re.compile('(К|к)ак день прошли', re.U), r'\g<1>ак день прошел'),
}


_SPLITS = [
    (re.compile(r'(\w)-то', re.U), r'\g<1> - то')
]


class RespectRewriter(object):
    MASC_GENDER = 'masc'
    FEMN_GENDER = 'femn'

    _YOU_MAPPING = [
        ('ты', 'вы'),
        ('тебя', 'вас'),
        ('тебе', 'вам'),
        ('тобою', 'вами'),
        ('тобой', 'вами'),
        ('тебе', 'вас')
    ]
    _LOCATIVE_PREPOSITIONS = {'в', 'о', 'об', 'на', 'при'}

    def __init__(self, parser_path, to_plural, to_gender=MASC_GENDER,
                 classifier_path=None, tagger_path=None, embeddings_path=None):
        if os.path.isdir(parser_path):
            self._parser = Parser.load(parser_path)
        elif os.path.isfile(parser_path):
            self._parser = Parser.load_from_archive(parser_path)
        else:
            assert False

        self._classifier = None
        if classifier_path and os.path.isfile(classifier_path):
            assert os.path.isfile(embeddings_path)
            self._classifier = RewritableSentenceClassifier.load_from_archive(classifier_path, embeddings_path)

        self._tagger = None
        if tagger_path and os.path.isfile(tagger_path):
            assert os.path.isfile(embeddings_path)
            self._tagger = RewritablePositionsClassifier.load_from_archive(tagger_path, embeddings_path)

        self._split_punct = re.compile('([{}])([ {}])'.format(punctuation, punctuation), re.U)
        self._split_particles = re.compile(r'(\w)-(то|либо|нибудь)', re.U)

        self._to_plural = to_plural

        self._from_number = 'Sing' if to_plural else 'Plur'
        self._to_number_pymorphy = 'plur' if to_plural else 'sing'
        self._to_gender = None if to_plural else to_gender

        self._is_second_person_verb = re.compile('(?:VERB|AUX).*Number={}.*Person=2.*'.format(self._from_number))

        self._you = 'ты' if to_plural else 'вы'
        self._your = 'твой' if to_plural else 'ваш'
        self._yourself = 'сам'
        self._your_parse = self._parser.morph.parse('ваш' if to_plural else 'твой')[0]

        self._you_in_prep_case_source = 'тебе' if to_plural else 'вас'
        self._you_in_prep_case_target = 'вас' if to_plural else 'тебе'
        self._you_in_prep_case_other = 'вам' if to_plural else 'тебя'

        self._you_mapping = {}
        for singular_form, plural_form in self._YOU_MAPPING:
            if to_plural:
                self._you_mapping[singular_form] = plural_form
            else:
                self._you_mapping[plural_form] = singular_form

    def _should_return_fast(self, tokens):
        parse = self._parser.parse(tokens)

        for grammar_val, lemma in zip(parse['grammar_values'], parse['lemmas']):
            if 'Person=2' in grammar_val:
                return False
            if re.match('(?:VERB|AUX).*Number=Sing|Tense=Past', grammar_val):
                return False
            if lemma in {self._you, self._your}:
                return False

        return True

    def _process_sentence(self, sentence, process_question):
        tokens = self._tokenize(sentence)
        token_texts = [token.text for token in tokens]

        if self._should_return_fast(token_texts):
            return sentence

        if self._classifier:
            if self._classifier.predict_proba([token_texts])[0] < 0.5:
                return sentence
        elif process_question:
            return sentence

        parse = self._parser.parse(token_texts, predict_syntax=True)

        should_rewrite_probs = np.ones(len(token_texts))
        if self._tagger:
            should_rewrite_probs = self._tagger.predict_proba(token_texts)

        result = ''
        for token_index in range(len(tokens)):
            if token_index > 0:
                result += sentence[tokens[token_index - 1].stop : tokens[token_index].start]

            if should_rewrite_probs[token_index] > 0.5:
                word = self._process_word(token_index, parse, token_texts, process_question=process_question)
            else:
                word = token_texts[token_index]

            result += self._restore_capitalization(token_texts[token_index], word)
        result += sentence[tokens[-1].stop:]

        return result

    @staticmethod
    def _tokenize(text):
        return list(tokenize(text, splits=_SPLITS))

    @staticmethod
    def _restore_capitalization(original_word, result_word):
        if original_word[0].lower() != original_word[0]:
            if result_word[0].lower() == result_word[0]:
                return result_word.capitalize()
        return result_word

    def _should_process(self, token_index, parse, tokens, process_question):
        nsubj = self._get_nsubj(token_index, parse)
        if nsubj is not None or parse['head_tags'][token_index] in ('root', 'parataxis'):
            if nsubj == self._you:
                return True

            if process_question and nsubj is None:
                for dependant_index in self._get_dependants(token_index, parse):
                    if parse['lemmas'][dependant_index] == self._yourself:
                        return True
                return self._is_question_clause(token_index, parse, tokens)
            return False

        if self._depends_on_second_person(token_index, parse):
            return True

        if parse['head_tags'][token_index] in ('cop', 'conj', 'advcl', 'acl', 'ccomp'):
            return self._should_process(parse['heads'][token_index] - 1, parse, tokens, process_question)

        return False

    @staticmethod
    def _get_dependants(token_index, parse):
        for cur_token_index, head_index in enumerate(parse['heads']):
            if head_index == token_index + 1:
                yield cur_token_index

    @staticmethod
    def _get_nsubj(token_index, parse):
        for index in RespectRewriter._get_dependants(token_index, parse):
            if parse['head_tags'][index].startswith('nsubj'):
                return parse['lemmas'][index]

    def _depends_on_second_person(self, token_index, parse):
        parent_index = parse['heads'][token_index] - 1
        if parse['lemmas'][parent_index] == self._you:
            return True

        if self._is_second_person_verb.match(parse['grammar_values'][parent_index]):
            return True

        return False

    @staticmethod
    def _is_question_clause(token_index, parse, tokens):
        for token, pos in zip(tokens[token_index:], parse['grammar_values'][token_index:]):
            if pos.startswith('PUNCT'):
                return token == '?'
        return False

    def _should_process_you(self, token_index, parse, tokens):
        if tokens[parse['heads'][token_index] - 1].lower() == 'ух':
            return False

        for other_token_index, head_index in enumerate(parse['heads']):
            if tokens[other_token_index].lower() != 'ух':
                continue
            if head_index == token_index + 1:
                return False

        return True

    def _process_word(self, token_index, parse, tokens, process_question):
        self._fix_parse(token_index, parse, tokens)

        grammar_val = parse['grammar_values'][token_index]

        if parse['lemmas'][token_index] == self._your:
            return self._process_your(token_index, parse, tokens)

        if ('Number=' + self._from_number) not in grammar_val:
            return tokens[token_index]

        if parse['lemmas'][token_index] == self._you and self._should_process_you(token_index, parse, tokens):
            return self._process_you(token_index, parse, tokens)

        should_process = self._should_process(token_index, parse, tokens, process_question)

        if grammar_val.startswith('VERB') or grammar_val.startswith('AUX'):
            if 'Person' in grammar_val:
                is_correct_form = ('Mood=Imp' in grammar_val) or should_process
                if is_correct_form and 'Person=2' in grammar_val:
                    return self._inflect_word(tokens[token_index], grammar_val, force_feats=['Person=2'])
            elif should_process and 'Gender=Neut' not in grammar_val:
                return self._inflect_word(tokens[token_index], grammar_val, force_feats=[])
        elif grammar_val.startswith('ADJ') and should_process:
            if 'Variant=Short' in grammar_val and 'Gender=Neut' not in grammar_val:
                return self._inflect_word(tokens[token_index], grammar_val, force_feats=['Variant=Short'])
            elif parse['lemmas'][token_index] == self._yourself:
                return self._inflect_word(tokens[token_index], grammar_val, force_feats=[], force_pos=False)

        return tokens[token_index]

    def _fix_parse(self, token_index, parse, tokens):
        if tokens[token_index] == 'права':
            nsubj = self._get_nsubj(token_index, parse)
            if parse['head_tags'][token_index] == 'root' and nsubj == self._you:
                parse['grammar_values'][token_index] = 'ADJ|Degree=Pos|Gender=Fem|Number=Sing|Variant=Short'

    def _process_you(self, token_index, parse, tokens):
        if tokens[token_index].lower() == self._you_in_prep_case_source:
            if 'Case=Loc' in parse['grammar_values'][token_index]:
                return self._you_in_prep_case_target

            for cur_token_index, head_index in enumerate(parse['heads']):
                if head_index == token_index + 1 and tokens[cur_token_index].lower() in self._LOCATIVE_PREPOSITIONS:
                    return self._you_in_prep_case_target

            return self._you_in_prep_case_other

        return self._you_mapping.get(tokens[token_index].lower(), tokens[token_index])

    def _process_your(self, token_index, parse, tokens):
        morph_parse = self._parser.get_pymorphy_form(tokens[token_index], parse['grammar_values'][token_index])

        if morph_parse:
            your_parse = self._your_parse.inflect(morph_parse.tag.grammemes)
            if your_parse:
                return your_parse.word
        logger.warning('Cannot process %s', tokens[token_index])
        return tokens[token_index]

    def _inflect_word(self, token, grammar_value, force_feats=None, force_pos=True):
        morpho_parse = self._parser.get_pymorphy_form(token, grammar_value, force_feats, force_pos)
        if morpho_parse is None:
            return token

        morpho_parse = morpho_parse.inflect({self._to_number_pymorphy})
        if morpho_parse:
            if morpho_parse.tag.gender and self._to_gender:
                morpho_parse = morpho_parse.inflect({self._to_gender}) or morpho_parse
            if morpho_parse:
                return morpho_parse.word

        logger.warning('Cannot inflect %s', token)
        return token

    def _preprocess(self, text):
        text = six.ensure_text(text)
        return text

    @staticmethod
    def _postprocess(text):
        for pattern, sub in _PLURALIZER_POSTPROCESS_FIXLIST:
            text = pattern.sub(sub, text)
        return text

    def may_be_rewritten(self, text):
        if not self._classifier:
            return False

        text = self._preprocess(text)
        for sentence in sentenize(text):
            tokens = self._tokenize(sentence.text)
            token_texts = [token.text for token in tokens]

            if self._classifier.predict_proba([token_texts])[0] >= 0.5:
                return True
        return False

    def __call__(self, original_text):
        text = self._preprocess(original_text)

        if self._to_plural and (text in _PLURALIZER_FIXLIST):
            result = _PLURALIZER_FIXLIST[text]
        elif not self._to_plural and (text in _SINGULARIZER_FIXLIST):
            result = _SINGULARIZER_FIXLIST[text]
        else:
            result = self._process_text(text)

        if isinstance(original_text, six.binary_type):
            result = six.ensure_binary(result)

        return result

    def _process_text(self, text):
        result = ''
        sentences = list(sentenize(text))
        for i, sentence in enumerate(sentences):
            if i > 0:
                result += text[sentences[i - 1].stop : sentences[i].start]

            rewritten_sentence = self._process_sentence(sentence.text, process_question=False)
            if rewritten_sentence.endswith('?'):
                rewritten_sentence = self._process_sentence(rewritten_sentence, process_question=True)
            rewritten_sentence = self._postprocess(rewritten_sentence)
            result += rewritten_sentence

        result += text[sentences[-1].stop:]

        return result
