import json
import io
import sys
import argparse
import random
import re
import codecs
from collections import defaultdict
from statistics import mean
import pymorphy2
from pylev import wfi_levenshtein


morph = pymorphy2.MorphAnalyzer()


DEFAULT_TAG = 'asr'
DEL_PENALTY = 1
INS_PENALTY = 1
SUB_PENALTY = 1


POS_DICT = {
    "FOREIGN": ["LATN"],  # латиница
    "MISC": ["NPRO",  # местоимение-существительное (он)
             "PREP",  # предлог (в)
             "CONJ",  # союз (и)
             "INTJ",  # междометие (ой)
             "PRCL",  # частица (бы, же, лишь)
             "UNKN",
             "null"],
    "NOUN": ["NOUN"],  # имя существительное (хомяк)
    "VERB": ["VERB",  # глагол (личная форма, инфинитив INFN)
             "INFN"],  # (говорю, говорит, говорил, говорить, сказать)
    "ADVB": ["ADVB",  # наречие (круто)
             "COMP",  # компаратив COMP (лучше, получше, выше)
             "PRED",  # предикатив PRED (некогда)
             "GRND"],  # деепричастие (прочитав, рассказывая)
    "ADJ": ["ADJF",  # имя прилагательное (полное ADJF, краткое ADJS)
            "ADJS",  # (хороший, хорош)
            "PRTF",  # причастие (полное PRTF, краткое PRTS)
            "PRTS"],  # (прочитавший, прочитанная, прочитана)
    "NUMR": ["NUMR",  # числительное (три, пятьдесят)
             "NUMB",  # числительное цифрами (3, 50)
             "intg",
             "real",  # вещественное число (3.14)
             "ROMN"]  # римское число (XI)
    }


PROPER_NOUN_TAGS = {'Abbr', 'Name', 'Surn', 'Patr', 'Geox', 'Orgn', 'Trad'}


PROB_THRESH = 0.4  # probability score threshold


STOP_PROPER_NOUNS = {
    'алиса', 'яндекс', 'алис', 'алисочка',
    'алисонька', 'алиска', 'олеся', 'сири'
    }


TRANSLITERATE_DICT = {
    'а': 'a', 'б': 'b', 'в': 'v', 'г': 'g', 'д': 'd', 'е': 'e', 'ё': 'e',
    'ж': 'zh', 'з': 'z', 'и': 'i', 'й': 'i', 'к': 'k', 'л': 'l', 'м': 'm',
    'н': 'n', 'о': 'o', 'п': 'p', 'р': 'r', 'с': 's', 'т': 't', 'у': 'u',
    'ф': 'f', 'х': 'h', 'ц': 'c', 'ч': 'ch', 'ш': 'sh', 'щ': 'sch', 'ъ': '',
    'ы': 'y', 'ь': '', 'э': 'e', 'ю': 'u', 'я': 'ya', 'А': 'A', 'Б': 'B',
    'В': 'V', 'Г': 'G', 'Д': 'D', 'Е': 'E', 'Ё': 'E', 'Ж': 'ZH', 'З': 'Z',
    'И': 'I', 'Й': 'I', 'К': 'K', 'Л': 'L', 'М': 'M', 'Н': 'N', 'О': 'O',
    'П': 'P', 'Р': 'R', 'С': 'S', 'Т': 'T', 'У': 'U', 'Ф': 'F', 'Х': 'H',
    'Ц': 'C', 'Ч': 'CH', 'Ш': 'SH', 'Щ': 'SCH', 'Ъ': '', 'Ы': 'Y', 'Ь': '',
    'Э': 'E', 'Ю': 'U', 'Я': 'YA', ',': '', '?': '', ' ': '_', '~': '',
    '!': '', '@': '', '#': '', '$': '', '%': '', '^': '', '&': '', '*': '',
    '(': '', ')': '', '-': '', '=': '', '+': '', ':': '', ';': '', '<': '',
    '>': '', '\'': '', '"': '', '\\': '', '/': '', '№': '', '[': '', ']': '',
    '{': '', '}': '', 'ґ': '', 'ї': '', 'є': '', 'Ґ': 'g', 'Ї': 'i', 'Є': 'e',
    '—': ''
    }


V00_WEIGHTS = {
    "Del_FOREIGN_norm": 2.78,
    "Del_NOUN_norm": 1.9,
    "Ins_FOREIGN_norm": 1.74,
    "Del_ADJ_norm": 1.32,
    "Ins_NOUN_norm": 1.34,
    "Ins_ADJ_norm": 1.2,
    "Ins_VERB_norm": 0.79,
    "Del_NUMR_norm": 0.66,
    "Sub_NOUN_norm": 0.6,
    "Sub_FOREIGN_norm": 0.45,
    "Sub_NUMR_norm": 0.31,
    "Ins_NUMR_norm": -0.27,
    "Sub_ADVB_norm": -0.25,
    "Del_VERB_norm": -0.28,
    "Sub_MISC_norm": -0.32,
    "Sub_VERB_norm": -0.55,
    "Ins_MISC_norm": -0.46,
    "Del_MISC_norm": -0.37,
    "Sub_ADJ_norm": -0.71,
    "Del_ADVB_norm": -0.86,
    "Ins_ADVB_norm": -1.45
}


V11_WEIGHTS = {
    "Del_NUMR": 4.50,
    "Del_NOUN": 2.50,
    "Del_FOREIGN": 2.50,
    "Del_ADJ": 2.50,
    "Del_proper_nouns": 1.00,
    "Ins_FOREIGN": 1.00,
    "Ins_NOUN": 1.00,
    "Sub_VERB": 1.00,
    "Del_VERB": 1.00,
    "Sub_NUMR": 1.00,
    "Sub_sense": 1.00,
    "Sub_proper_nouns": 1.00,
    "Sub_NOUN": 1.00,
    "Ins_proper_nouns": 1.00,
    "Del_sense": 1.00,
    "Ins_sense": 1.00,
    "Sub_nonsense": -1.00,
    "Ins_nonsense": -1.00,
    "Del_nonsense": -1.00
}


def has_cyrillic(text: str) -> bool:
    '''
    Возвращает True, если в строке есть кириллица.
    '''
    return bool(re.search('[а-яА-Я]', text))


def transliterate(input_word: str) -> str:
    '''
    Транслитерирует слово на кириллице.
    '''
    if not has_cyrillic(input_word):
        return input_word
    for key in TRANSLITERATE_DICT:
        input_word = input_word.replace(key, TRANSLITERATE_DICT[key])
    return input_word


def count_proper_noun(word_list: list) -> int:
    '''
    Считает имена собственные в списке слов.
    Не считает за имена собственные слова из STOP_PROPER_NOUNS.
    '''
    count = 0
    for word in word_list:
        if word in STOP_PROPER_NOUNS:
            continue
        for p in morph.parse(word):
            for tag in PROPER_NOUN_TAGS:
                if (tag in p.tag) and (p.score >= PROB_THRESH):
                    count += 1
                    break
    return count


def replace_cluster_references(text: str, cluster_references: dict) -> str:
    '''
    Заменяет кластерные референсы в строке.
    В качестве cluster_references требуется словарь вида {"reference1": ["hyp1", hyp2, ..], ..}.
    Например {"rammstein": ["рамштайн", "раммштайн"], ..}.
    '''
    text = ' {} '.format(text)
    for reference_word in cluster_references.keys():
        for word in cluster_references[reference_word]:
            text = text.replace(' {} '.format(word), ' {} '.format(reference_word))
    return text.strip()


def lemm_recognizer(word: str) -> str:
    '''
    Возвращает начальную форму передающегося слова.
    '''
    word_parse = morph.parse(word)
    max_score = 0
    for form in word_parse:
        if form.score > max_score:
            max_score = form.score
            choosen_form = form
    lemm = choosen_form.normal_form
    return lemm


def lemmatizer(text: str) -> str:
    '''
    Заменяет все слова во передающемся предложении на свою начальную форму.
    '''
    tokens = text.split(' ')
    return ' '.join([
        lemm_recognizer(token) for token in tokens
    ])


def part_of_speech_recognizer(word: str) -> str:
    '''
    Возвращает часть речи передающегося слова.
    '''
    word_parse = morph.parse(word)
    max_score = 0
    for form in word_parse:
        if form.score > max_score:
            max_score = form.score
            choosen_form = form
    form_tag = str(choosen_form.tag)
    for key in POS_DICT.keys():
        for pos in POS_DICT[key]:
            if pos in form_tag:
                return key
    return form_tag


def part_of_speech_dict(word_list: list, levenshtein=False) -> dict:
    if levenshtein:
        output_part_of_speech_dict = defaultdict(list)
    else:
        output_part_of_speech_dict = defaultdict(int)
    for word in word_list:
        if levenshtein:
            cur_word, lev_value = word
            part_of_speech = part_of_speech_recognizer(cur_word)
            output_part_of_speech_dict[part_of_speech].append(lev_value)
        else:
            cur_word = word
            part_of_speech = part_of_speech_recognizer(cur_word)
            output_part_of_speech_dict[part_of_speech] += 1
    return dict(output_part_of_speech_dict)


def get_n_words(i):
    if isinstance(i, list):
        res = len(i)
    elif isinstance(i, dict):
        res = 0
        for k in i:
            res += get_n_words(i[k])
    else:
        res = i
    return res


def wer_star(metrics: dict, wer_star_v00_weights: dict, wer_star_v11_weights: dict, wer_type='') -> dict:
    '''
    Вычисляет WER*v00 и WER*v11
    В качестве аргумента metrics требуется словарь, возвращаемый функцией wer_components
    wer_star_v00_weights и wer_star_v11_weights соответственно веса для каждой вариации WER* (задаются json'ом в Нирване)
    wer_type - тип WERa, отраженный в названии компонент (пока что '' или Lemm)
    (например для WER(lemm) все компоненты будут начинаться с Lemm: LemmIns_sense, LemmDel_POS и т.д.)
    '''

    simple_metrics = []
    wer_star_components_dict = {}
    for k in metrics.keys():
        if not (isinstance(metrics[k], dict) or isinstance(metrics[k], list)):
            simple_metrics.append(k)
    for metric in simple_metrics:
        if (not wer_type) or metric.startswith(wer_type):
            wer_star_components_dict[metric] = metrics[metric]
    components = ['Del', 'Ins', 'Sub']
    for component in components:
        component = wer_type + component
        metric = component + "_words_len"
        name = component + "_avg_len"
        if metrics[metric]:
            wer_star_components_dict[name] = mean(metrics[metric])
        else:
            wer_star_components_dict[name] = 0

        # add normed versions for _proper_nouns, _sense, _nonsense

        name = component + "_norm"
        wer_star_components_dict[name] = metrics[component] / metrics['N']

        metric = component + "_POS"
        metric_norm = metric + "_norm"
        for pos in POS_DICT:
            name = '{}_{}'.format(component, pos)
            wer_star_components_dict[name] = get_n_words({i: metrics[metric][i]
                                                         for i in metrics[metric]
                                                         if i == pos})
            name += "_norm"
            wer_star_components_dict[name] = get_n_words({i: metrics[metric][i]
                                                          for i in metrics[metric]
                                                          if i == pos}) / float(metrics['N'])
            if component == wer_type + 'Sub':
                if pos in metrics[metric_norm].keys():
                    name = '{}_{}_dist'.format(component, pos)
                    wer_star_components_dict[name] = mean([j for i in metrics[metric_norm]
                                                           for j in metrics[metric_norm][i]
                                                           if i == pos])
                    name += "_norm"
                    wer_star_components_dict[name] = mean([j for i in metrics[metric_norm]
                                                           for j in metrics[metric_norm][i]
                                                           if i == pos]) * metrics[component] / metrics['N']
                else:
                    name = '{}_{}_dist'.format(component, pos)
                    wer_star_components_dict[name] = 0
                    name += "_norm"
                    wer_star_components_dict[name] = 0
    weighted_components_v00 = [wer_star_components_dict[wer_type+component] * wer_star_v00_weights[component]
                               for component in wer_star_v00_weights]
    wer_star_v00 = sum(weighted_components_v00)
    weighted_components_v11 = [wer_star_components_dict[wer_type+component] * wer_star_v11_weights[component]
                               for component in wer_star_v11_weights]
    wer_star_v11 = sum(weighted_components_v11) / float(metrics['N'])
    return wer_star_v00, wer_star_v11


def wer_components(ref: str, hyp: str,
                   stop_words: set, sense_words: set) -> dict:
    '''
    Возвращает словарь с компонентами WER и их описанием (части речи, индикаторы стоп-слов и проч.)
    '''
    reference_words = re.findall(r'\w+', ref)
    hypothesis_words = re.findall(r'\w+', hyp)
    costs = [[0 for inner in range(len(hypothesis_words) + 1)]
             for outer in range(len(reference_words) + 1)]
    backtrace = [[0 for inner in range(len(hypothesis_words) + 1)]
                 for outer in range(len(reference_words) + 1)]

    OP_OK = 0
    OP_SUB = 1
    OP_INS = 2
    OP_DEL = 3

    for i in range(1, len(reference_words) + 1):
        costs[i][0] = DEL_PENALTY * i
        backtrace[i][0] = OP_DEL

    for j in range(1, len(hypothesis_words) + 1):
        costs[0][j] = INS_PENALTY * j
        backtrace[0][j] = OP_INS

    for i in range(1, len(reference_words) + 1):
        for j in range(1, len(hypothesis_words) + 1):
            if reference_words[i - 1] == hypothesis_words[j - 1]:
                costs[i][j] = costs[i - 1][j - 1]
                backtrace[i][j] = OP_OK
            else:
                substitutionCost = costs[i - 1][j - 1] + SUB_PENALTY
                insertionCost = costs[i][j - 1] + INS_PENALTY
                deletionCost = costs[i - 1][j] + DEL_PENALTY
                costs[i][j] = min(substitutionCost,
                                  insertionCost,
                                  deletionCost)
                if costs[i][j] == substitutionCost:
                    backtrace[i][j] = OP_SUB
                elif costs[i][j] == insertionCost:
                    backtrace[i][j] = OP_INS
                else:
                    backtrace[i][j] = OP_DEL
    components_dict = {}
    components_dict['Ins_sense'] = 0
    components_dict['Ins_nonsense'] = 0
    components_dict['Del_sense'] = 0
    components_dict['Del_nonsense'] = 0
    components_dict['Cor_sense'] = 0
    components_dict['Cor_nonsense'] = 0
    components_dict['Sub_sense'] = 0
    components_dict['Sub_nonsense'] = 0
    components_dict['Del_words'] = []
    components_dict['Cor_words'] = []
    components_dict['Ins_words'] = []
    components_dict['Sub_words'] = []
    part_of_speech_match_counter = 0
    lemm_match_counter = 0
    sub_words_lev = []
    sub_words_norm_lev = []
    i = len(reference_words)
    j = len(hypothesis_words)
    while i > 0 or j > 0:
        if backtrace[i][j] == OP_OK:
            components_dict['Cor_words'].append(reference_words[i - 1])
            if reference_words[i - 1] in sense_words:
                components_dict['Cor_sense'] += 1
            elif reference_words[i - 1] in stop_words:
                components_dict['Cor_nonsense'] += 1
            i -= 1
            j -= 1
        elif backtrace[i][j] == OP_SUB:
            ref_word = reference_words[i - 1]
            hyp_word = hypothesis_words[j - 1]
            if not (has_cyrillic(hyp_word) and has_cyrillic(ref_word)):
                transliterated_hyp_word = transliterate(hyp_word)
                transliterated_ref_word = transliterate(ref_word)
                lev_dist = wfi_levenshtein(transliterated_hyp_word,
                                                transliterated_ref_word)
            else:
                lev_dist = wfi_levenshtein(hyp_word, ref_word)
            if part_of_speech_recognizer(hyp_word) == part_of_speech_recognizer(ref_word):
                part_of_speech_match_counter += 1
            if lemm_recognizer(hyp_word) == lemm_recognizer(ref_word):
                lemm_match_counter += 1
            components_dict['Sub_words'].append(ref_word)
            norm_lev_dist = lev_dist / len(ref_word)
            sub_words_lev.append((ref_word, lev_dist))
            sub_words_norm_lev.append((ref_word, norm_lev_dist))
            if ref_word in sense_words:
                components_dict['Sub_sense'] += 1
            elif ref_word in stop_words:
                components_dict['Sub_nonsense'] += 1
            i -= 1
            j -= 1
        elif backtrace[i][j] == OP_INS:
            components_dict['Ins_words'].append(hypothesis_words[j - 1])
            if hypothesis_words[j - 1] in sense_words:
                components_dict['Ins_sense'] += 1
            elif hypothesis_words[j - 1] in stop_words:
                components_dict['Ins_nonsense'] += 1
            j -= 1
        elif backtrace[i][j] == OP_DEL:
            components_dict['Del_words'].append(reference_words[i - 1])
            if reference_words[i - 1] in sense_words:
                components_dict['Del_sense'] += 1
            elif reference_words[i - 1] in stop_words:
                components_dict['Del_nonsense'] += 1
            i -= 1
    components_dict['Sub'] = len(components_dict['Sub_words'])
    components_dict['Del'] = len(components_dict['Del_words'])
    components_dict['Ins'] = len(components_dict['Ins_words'])
    components_dict['Cor'] = len(components_dict['Cor_words'])
    components_dict['Ins_proper_nouns'] = count_proper_noun(components_dict['Ins_words'])
    components_dict['Del_proper_nouns'] = count_proper_noun(components_dict['Del_words'])
    components_dict['Cor_proper_nouns'] = count_proper_noun(components_dict['Cor_words'])
    components_dict['Sub_proper_nouns'] = count_proper_noun(components_dict['Sub_words'])
    components_dict['Сor_POS'] = part_of_speech_dict(components_dict['Cor_words'])
    components_dict['Ins_POS'] = part_of_speech_dict(components_dict['Ins_words'])
    components_dict['Del_POS'] = part_of_speech_dict(components_dict['Del_words'])
    components_dict['Sub_POS'] = part_of_speech_dict(sub_words_lev, True)
    components_dict['Sub_POS_norm'] = part_of_speech_dict(sub_words_norm_lev, True)
    components_dict['Сor_words_len'] = [len(word) for word in components_dict['Cor_words']]
    components_dict['Ins_words_len'] = [len(word) for word in components_dict['Ins_words']]
    components_dict['Del_words_len'] = [len(word) for word in components_dict['Del_words']]
    components_dict['Sub_words_len'] = [len(word) for word in components_dict['Sub_words']]
    if components_dict['Sub']:
        components_dict['Part_of_speech_matches_ratio'] = part_of_speech_match_counter / components_dict['Sub']
        components_dict['Lemm_matches_ratio'] = lemm_match_counter / components_dict['Sub']
    else:
        components_dict['Part_of_speech_matches_ratio'] = 0
        components_dict['Lemm_matches_ratio'] = 0
    return components_dict


def calculate_metrics(ref: str, hyp: str, stop_words: set, sense_words: set, cluster_references: dict,
                        WER_star_v00_weights: dict, WER_star_v11_weights: dict) -> dict:
    '''
    Возвращает общий словарь всех метрик + компоненты WER и WER(lemm)
    '''

    WER_star_v00_weights = WER_star_v00_weights or V00_WEIGHTS
    WER_star_v11_weights = WER_star_v11_weights or V11_WEIGHTS

    basic_wer_components = wer_components(ref, hyp, stop_words, sense_words)
    cr_ref = replace_cluster_references(ref, cluster_references)
    cr_hyp = replace_cluster_references(hyp, cluster_references)
    lemm_ref = lemmatizer(cr_ref)
    lemm_hyp = lemmatizer(cr_hyp)
    lemm_wer_components = wer_components(lemm_ref, lemm_hyp, stop_words, sense_words)
    metrics_dict = {}
    metrics_keys = {
        'CER',
        'WER(hunt)',
        'WER(basic)',
        'WER(lemm)',
        'MER',
        'WIL',
        'Micro_avg_precision',
        'Macro_avg_precision',
        'Micro_avg_recall',
        'Macro_avg_recall',
        'Micro_F_measure',
        'Macro_F_measure'
    }
    for m in metrics_keys:
        metrics_dict[m] = 0.0
    metrics_dict['CER'] = 1.0 * wfi_levenshtein(ref, hyp)/len(ref)
    reference_words = re.findall(r'\w+', ref)
    hypothesis_words = re.findall(r'\w+', hyp)
    metrics_dict['N'] = len(reference_words) or 1.0
    reference_words_set = set(reference_words)
    hypothesis_words_set = set(hypothesis_words)
    Cor = basic_wer_components['Cor']
    Ins = basic_wer_components['Ins']
    Del = basic_wer_components['Del']
    Sub = basic_wer_components['Sub']
    if hypothesis_words_set:
        recall_sum = 0
        for word in hypothesis_words_set:
            recall_sum += basic_wer_components['Cor_words'].count(word) / hypothesis_words.count(word)
        metrics_dict['Macro_avg_recall'] = recall_sum / len(hypothesis_words_set)
    if reference_words_set:
        precision_sum = 0
        for word in reference_words_set:
            precision_sum += basic_wer_components['Cor_words'].count(word) / reference_words.count(word)
        metrics_dict['Macro_avg_precision'] = precision_sum / len(reference_words_set)
    if metrics_dict['Macro_avg_precision'] and metrics_dict['Micro_avg_recall']:
        metrics_dict['Macro_F_measure'] = (2 * metrics_dict['Macro_avg_precision'] * metrics_dict['Macro_avg_recall']) \
        / (metrics_dict['Macro_avg_precision'] + metrics_dict['Macro_avg_recall'])
    if reference_words:
        metrics_dict['Micro_avg_precision'] = Cor / len(reference_words)
        metrics_dict['WER(hunt)'] = round((Sub + 0.5 * (Del + Ins)) / len(reference_words), 3)
        metrics_dict['WER(basic)'] = round((Sub + Del + Ins) / len(reference_words), 3)
        metrics_dict['WER(lemm)'] = round((lemm_wer_components['Sub'] + lemm_wer_components['Del'] + lemm_wer_components['Ins']) / len(reference_words), 3)
        metrics_dict['MER'] = round((Sub + Del + Ins) / (Sub + Del + Ins + Cor), 3)
    elif hypothesis_words:
        metrics_dict['WER(hunt)'] = 1.0
        metrics_dict['WER(basic)'] = 1.0
        metrics_dict['MER'] = 1.0
    if hypothesis_words:
        metrics_dict['Micro_avg_recall'] = Cor / len(hypothesis_words)
    if hypothesis_words and reference_words:
        metrics_dict['WIL'] = round(1.0 - Cor**2 / (len(reference_words) * len(hypothesis_words)), 3)
    if metrics_dict['Micro_avg_recall'] or metrics_dict['Micro_avg_precision']:
        metrics_dict['Micro_F_measure'] = (2 * metrics_dict['Micro_avg_recall'] * metrics_dict['Micro_avg_precision']) \
        / (metrics_dict['Micro_avg_precision'] + metrics_dict['Micro_avg_recall'])
    for feature in basic_wer_components.keys():
        metrics_dict[feature] = basic_wer_components[feature]
        metrics_dict['Lemm' + feature] = lemm_wer_components[feature]
    metrics_dict['WER*v00'], metrics_dict['WER*v11'] = wer_star(metrics_dict,
                                                                WER_star_v00_weights,
                                                                WER_star_v11_weights)
    return metrics_dict


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_table')
    parser.add_argument('--reference_input_column')
    parser.add_argument('--id_input_column')
    parser.add_argument('--hypothesis_input_column')
    parser.add_argument('--cluster_references')
    parser.add_argument('--stop_words')
    parser.add_argument('--wer_star_v00_weights')
    parser.add_argument('--wer_star_v11_weights')
    parser.add_argument('--sense_words')
    parser.add_argument('--output_table')
    parser.add_argument('--records_json')
    parser.add_argument('--recognize_json')
    args = parser.parse_args(argv[1:])
    with codecs.open(args.input_table, 'r', 'utf-8-sig') as f:
        input_table = json.load(f)
    with io.open(args.sense_words, 'r') as f:
        sense_words = json.load(f)
    with io.open(args.stop_words, 'r') as f:
        stop_words = json.load(f)
    if args.wer_star_v00_weights:
        with io.open(args.wer_star_v00_weights, 'r') as f:
            wer_star_v00_weights = json.load(f)
    else:
        wer_star_v00_weights = None
    if args.wer_star_v11_weights:
        with io.open(args.wer_star_v11_weights, 'r') as f:
            wer_star_v11_weights = json.load(f)
    else:
        wer_star_v11_weights = None
    with io.open(args.cluster_references, 'r') as f:
        cluster_references = json.load(f)
    stop_words = set(stop_words)
    sense_words = set(sense_words)
    ref_input_column = args.reference_input_column
    hyp_input_column = args.hypothesis_input_column
    id_input_column = args.id_input_column
    records_result = []
    recognize_result = {}
    for row in input_table:
        current_reference = str(row[ref_input_column])
        current_id = str(row[id_input_column])
        current_hypothesis = str(row[hyp_input_column])
        records_result.append({"id": current_id, "tags": [DEFAULT_TAG],
                               "ref": current_reference})
        recognize_result[current_id] = current_hypothesis
        metrics = calculate_metrics(current_reference, current_hypothesis,
                                    stop_words, sense_words, cluster_references,
                                    wer_star_v00_weights, wer_star_v11_weights)
        row['metrics'] = metrics
    with io.open(args.output_table, 'w', encoding='utf-8') as f:
        f.write(json.dumps(input_table, ensure_ascii=False))
    with io.open(args.records_json, 'w', encoding='utf-8') as f:
        f.write(json.dumps(records_result, ensure_ascii=False))
    with io.open(args.recognize_json, 'w', encoding='utf-8') as f:
        f.write(json.dumps(recognize_result, ensure_ascii=False))


if __name__ == '__main__':
    main(sys.argv)
