import copy
import logging
import numpy as np
import pickle
import pymorphy2
from library.python import resource
from transliterate import translit
from typing import Dict, List, Mapping, Optional, Sequence, Tuple, Union

from alice.analytics.wer.lib import utils
from voicetech.asr.tools.g2p.cython.caching_applier import CachingG2PApplier


class WERP(object):
    def __init__(
        self,
        cwd: str,
        use_pos_aware_pher: bool = True,
        penalize_expensive_operations: bool = True
    ):
        """
        :param cwd: путь до бинарного файла для g2p
        :param use_pos_aware_pher: подстраивает веса операций в PhER под их позицию в слове, если True
        :param penalize_expensive_operations: максимально штрафует дорогие операции (например, вставка слова 'не')
        """
        self.use_pos_aware_pher = use_pos_aware_pher
        self.penalize_expensive_operations = penalize_expensive_operations

        self.phoneme_mapping = utils.get_phoneme_mapping()
        self.g2p_applier = CachingG2PApplier(cwd)
        self.morph = pymorphy2.MorphAnalyzer()
        self.sense_words = set(resource.find('/sense.txt').decode('utf-8').split("\n"))
        self.stop_words = set(resource.find('/stop.txt').decode('utf-8').split("\n"))
        # загрузка статического кэша для ускорения g2p
        self.g2p_static_cache = self.load_g2p_static_cache()
        # массив стоимостей замен фонем
        if use_pos_aware_pher:
            self.substitute_costs = utils.build_substitute_costs(1.4, 0.2, 0.1, 2)
            self.insert_costs = utils.build_insert_costs(1, 0.4)
        else:
            self.substitute_costs = utils.build_substitute_costs(2, 0.2, 0.1, 2)
            self.insert_costs = utils.build_insert_costs(1.4, 0.4)

    def __enter__(self):
        return self

    def __call__(self, ref: str, hyp: str) -> Dict[str, Tuple[float, Dict[str, str], Dict[str, str]]]:
        try:
            return self.get_score(ref, hyp)
        except Exception as e:
            raise RuntimeError('WERP failed on inputs ({}, {})'.format(ref, hyp)) from e

    def __exit__(self, exc_type, exc_value, traceback):
        pass

    def get_score(self, ref: str, hyp: str) -> Dict[str, Tuple[float, Dict[str, str], Dict[str, str]]]:
        """
        Считает WERP(ref, hyp)
        """
        ref = utils.remove_tags(ref)
        hyp = utils.remove_tags(hyp)
        werp_result, ref_reprs, hyp_reprs = self.get_all_representations(ref, hyp)
        if werp_result is not None:
            return {'score': werp_result, 'ref_reprs': ref_reprs, 'hyp_reprs': hyp_reprs}

        scores, ref_reprs, hyp_reprs = self.get_representation_scores(ref_reprs, hyp_reprs)
        penalized_repr, penalized_score = self.get_penalized_score(scores)

        penalized_score = min(1, penalized_score)
        return {'score': penalized_score, 'ref_reprs': ref_reprs, 'hyp_reprs': hyp_reprs}

    def get_meaningful_text(self, text: str) -> str:
        """Вычисляет meaningful представление текста: удаляет повторы и стоп-слова"""
        words = utils.get_words(text)
        meaningful_words = []
        for word in words:
            if word not in self.stop_words and word not in meaningful_words:
                meaningful_words.append(word)
        return ' '.join(meaningful_words)

    def get_g2p_representation(self, line_to_send: str) -> str:
        """Вычисляет g2p представление для строки line_to_send"""
        phonemes_list = []
        if not line_to_send.strip():
            return line_to_send
        for attempt in range(5):
            try:
                answer = []
                for word in line_to_send.split():
                    if word in self.g2p_static_cache:
                        answer.append(self.g2p_static_cache[word])
                    else:
                        answer.append(self.g2p_applier.transcribe(word)[0])

                # in: "привет"
                # out: [{"word":"привет","phonemes":["p rʲ i vʲ + e t","p rʲ i vʲ e t"],"scores":[-0.004014719278,-2.421007872]}]
                if not answer:
                    raise ValueError('Empty answer')
                for word in answer:
                    if word['phonemes']:
                        phonemes_normalized = utils.normalize_phonemes(self.phoneme_mapping, copy.deepcopy(word['phonemes']))
                        phonemes_weighted = utils.weigh_phonemes(phonemes_normalized, word['scores'])
                        phonemes_list.append(max(phonemes_weighted, key=phonemes_weighted.get))
                    else:
                        phonemes_list.append(translit(word['word'], 'ru', reversed=True).replace("'", ''))
                break
            except ValueError:
                logging.warning('Attempt {}, G2P error for text: {}'.format(str(attempt), line_to_send))
        return ' '.join(phonemes_list)

    def initialize_reprs(self, text: str) -> Dict[str, str]:
        """Инициализирует словарь представлений"""
        reprs = {'original': text}
        reprs['transliterated'] = translit(text, 'ru', reversed=True).replace("'", '')
        reprs['normalized'] = utils.normalize_words(self.morph, text)
        reprs['meaningful'] = self.get_meaningful_text(text)
        reprs['transliterated'] = self.remove_unknown_chars(reprs, 'transliterated', text)
        return reprs

    def get_all_representations(self, ref: str, hyp: str) -> Tuple[Optional[float], Dict[str, str], Dict[str, str]]:
        """Получает фонемные представления текстов из модели g2p и значение метрики, если оно посчиталось в процессе"""
        ref_reprs = self.initialize_reprs(ref)
        hyp_reprs = self.initialize_reprs(hyp)

        werp_result = None

        for repr_type in ['original', 'normalized', 'meaningful']:
            ref_reprs[repr_type] = self.get_g2p_representation(ref_reprs[repr_type])
            hyp_reprs[repr_type] = self.get_g2p_representation(hyp_reprs[repr_type])
            ref_reprs[repr_type] = self.remove_unknown_chars(ref_reprs, repr_type, ref)
            hyp_reprs[repr_type] = self.remove_unknown_chars(hyp_reprs, repr_type, hyp)
            if self.penalize_expensive_operations and repr_type == 'original':
                # если после выравнивания нашлись запретные операции, то штрафуем по-максимуму
                if utils.check_for_expensive_operations(ref, hyp, ref_reprs[repr_type], hyp_reprs[repr_type],
                                                        self.substitute_costs, self.insert_costs):
                    werp_result = 1.
                    break  # другие запросы в g2p не делаем для экономии времени
        return werp_result, ref_reprs, hyp_reprs

    @staticmethod
    def remove_unknown_chars(reprs: Mapping[str, str], repr_type: str, initial_text: str) -> str:
        """
        Очищает представление от нетипичных символов
        Если есть нетипичные символы, делает warning:
            WERP: removing unexpected chars ['-', '~'] in text "алиса - ~", representation: "alisa - ~" (type original)
        """
        clean_repr, unknown_chars = utils.remove_unknown_chars(reprs[repr_type])
        if unknown_chars:
            logging.warning(f'WERP: removing unexpected characters {unknown_chars} in '
                            f'text "{initial_text}", representation "{reprs[repr_type]}" (type {repr_type})')
        return clean_repr

    def get_representation_scores(self, ref_reprs: Mapping[str, str], hyp_reprs: Mapping[str, str]) -> List[Union[Dict[str, str], Dict[str, float]]]:
        """Вычисляет скоры для всех фонемных представлений"""
        scores = {}
        for repr_type in ref_reprs:
            scores[repr_type] = utils.get_weighted_distance(utils.get_words(ref_reprs[repr_type]),
                                                            utils.get_words(hyp_reprs[repr_type]),
                                                            self.substitute_costs,
                                                            self.insert_costs,
                                                            use_pos_aware_pher=self.use_pos_aware_pher)
        return scores, ref_reprs, hyp_reprs

    @staticmethod
    def penalize(score, penalty: float) -> float:
        """Штрафующая функция для скора представления"""
        return score * (1 + penalty) + penalty

    def get_penalized_score(self, scores: Mapping[str, float]) -> Tuple[str, float]:
        """Возвращает минимум из штрафующих функций по всем представлениям"""
        penalties = {
            'original': 0.0,
            'transliterated': 0.05,
            'normalized': 0.2,
            'meaningful': 0.3
        }
        penalized_scores = {}
        for repr_type in scores:
            penalized_scores[repr_type] = self.penalize(scores[repr_type], penalties[repr_type])
        penalized_repr = min(penalized_scores, key=penalized_scores.get)
        penalized_score = penalized_scores[penalized_repr]
        return penalized_repr, penalized_score

    def load_g2p_static_cache(self):
        logging.debug('Load g2p static cache')
        static_cache = pickle.loads(resource.find('/word_cache.pkl'))
        logging.debug('Cache size {}'.format(len(static_cache)))
        return static_cache
