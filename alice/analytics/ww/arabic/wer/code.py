import re
import json
from collections import defaultdict, Counter

from pylev import wfi_levenshtein

import unicodedata
from num2words import num2words
from tashaphyne import normalize as normalize_arabic


DEFAULT_TAG = 'asr'
DEL_PENALTY = 1
INS_PENALTY = 1
SUB_PENALTY = 1



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


def wer_components(ref: str, hyp: str) -> dict:
    '''
    Возвращает словарь с компонентами WER и их описанием (части речи, индикаторы стоп-слов и проч.)
    Взято с: https://a.yandex-team.ru/arc/trunk/arcadia/alice/analytics/wer/metrics_counter.py?rev=r9114337#L316
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
    # components_dict['Ins_sense'] = 0
    # components_dict['Ins_nonsense'] = 0
    # components_dict['Del_sense'] = 0
    # components_dict['Del_nonsense'] = 0
    # components_dict['Cor_sense'] = 0
    # components_dict['Cor_nonsense'] = 0
    # components_dict['Sub_sense'] = 0
    # components_dict['Sub_nonsense'] = 0
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
            # if reference_words[i - 1] in sense_words:
            #     components_dict['Cor_sense'] += 1
            # elif reference_words[i - 1] in stop_words:
            #     components_dict['Cor_nonsense'] += 1
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
            # if part_of_speech_recognizer(hyp_word) == part_of_speech_recognizer(ref_word):
            #     part_of_speech_match_counter += 1
            # if lemm_recognizer(hyp_word) == lemm_recognizer(ref_word):
            #     lemm_match_counter += 1
            components_dict['Sub_words'].append(ref_word)
            norm_lev_dist = lev_dist / len(ref_word)
            sub_words_lev.append((ref_word, lev_dist))
            sub_words_norm_lev.append((ref_word, norm_lev_dist))
            # if ref_word in sense_words:
            #     components_dict['Sub_sense'] += 1
            # elif ref_word in stop_words:
            #     components_dict['Sub_nonsense'] += 1
            i -= 1
            j -= 1
        elif backtrace[i][j] == OP_INS:
            components_dict['Ins_words'].append(hypothesis_words[j - 1])
            # if hypothesis_words[j - 1] in sense_words:
            #     components_dict['Ins_sense'] += 1
            # elif hypothesis_words[j - 1] in stop_words:
            #     components_dict['Ins_nonsense'] += 1
            j -= 1
        elif backtrace[i][j] == OP_DEL:
            components_dict['Del_words'].append(reference_words[i - 1])
            # if reference_words[i - 1] in sense_words:
            #     components_dict['Del_sense'] += 1
            # elif reference_words[i - 1] in stop_words:
            #     components_dict['Del_nonsense'] += 1
            i -= 1
    components_dict['Sub'] = len(components_dict['Sub_words'])
    components_dict['Del'] = len(components_dict['Del_words'])
    components_dict['Ins'] = len(components_dict['Ins_words'])
    components_dict['Cor'] = len(components_dict['Cor_words'])
    # components_dict['Ins_proper_nouns'] = count_proper_noun(components_dict['Ins_words'])
    # components_dict['Del_proper_nouns'] = count_proper_noun(components_dict['Del_words'])
    # components_dict['Cor_proper_nouns'] = count_proper_noun(components_dict['Cor_words'])
    # components_dict['Sub_proper_nouns'] = count_proper_noun(components_dict['Sub_words'])
    # components_dict['Сor_POS'] = part_of_speech_dict(components_dict['Cor_words'])
    # components_dict['Ins_POS'] = part_of_speech_dict(components_dict['Ins_words'])
    # components_dict['Del_POS'] = part_of_speech_dict(components_dict['Del_words'])
    # components_dict['Sub_POS'] = part_of_speech_dict(sub_words_lev, True)
    # components_dict['Sub_POS_norm'] = part_of_speech_dict(sub_words_norm_lev, True)
    components_dict['Сor_words_len'] = [len(word) for word in components_dict['Cor_words']]
    components_dict['Ins_words_len'] = [len(word) for word in components_dict['Ins_words']]
    components_dict['Del_words_len'] = [len(word) for word in components_dict['Del_words']]
    components_dict['Sub_words_len'] = [len(word) for word in components_dict['Sub_words']]
    # if components_dict['Sub']:
        # components_dict['Part_of_speech_matches_ratio'] = part_of_speech_match_counter / components_dict['Sub']
        # components_dict['Lemm_matches_ratio'] = lemm_match_counter / components_dict['Sub']
    # else:
    # components_dict['Part_of_speech_matches_ratio'] = 0
    # components_dict['Lemm_matches_ratio'] = 0
    return components_dict


def calculate_metrics(ref: str, hyp: str, cluster_references: dict) -> dict:
    '''
    Возвращает общий словарь всех метрик + компоненты WER и WER(lemm)
    Взято с: https://a.yandex-team.ru/arc/trunk/arcadia/alice/analytics/wer/metrics_counter.py?rev=r9114337#L451
    '''

    basic_wer_components = wer_components(ref, hyp)
    # cr_ref = replace_cluster_references(ref, cluster_references)
    # cr_hyp = replace_cluster_references(hyp, cluster_references)
    # lemm_ref = lemmatizer(cr_ref)
    # lemm_hyp = lemmatizer(cr_hyp)
    # lemm_wer_components = wer_components(lemm_ref, lemm_hyp)
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
        # metrics_dict['WER(lemm)'] = round((lemm_wer_components['Sub'] + lemm_wer_components['Del'] + lemm_wer_components['Ins']) / len(reference_words), 3)
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
        # metrics_dict['Lemm' + feature] = lemm_wer_components[feature]
    # metrics_dict['WER*v00'], metrics_dict['WER*v11'] = wer_star(metrics_dict,
    #                                                             WER_star_v00_weights,
    #                                                             WER_star_v11_weights)
    return metrics_dict


def normalize_searchtext(text):
    text = normalize_arabic.strip_tashkeel(text)
    text = normalize_arabic.strip_tatweel(text)
    text = normalize_arabic.normalize_lamalef(text)
    text = normalize_arabic.normalize_spellerrors(text)

    return text


def normalize(text):
    # взято с: https://a.yandex-team.ru/arc/trunk/arcadia/voicetech/asr/tools/arabic/normalizer/normalizer.py?rev=r9010334#L35
    text = text.lower()
    text = unicodedata.normalize('NFKC', text)

    if re.search('(\d+\.\d+)|(\d+\,\d+)', text):
        return

    text = re.sub('\d+', lambda match: f" {num2words(int(match.group(0)), lang='ar')} ", text)
    text = re.sub("[!\"',-.:;?`\u060C\u060D\u061B\u061E\u061F]", ' ', text)
    text = normalize_searchtext(text)
    text = re.sub('[ \t\n]+', ' ', text).strip()

    if not re.match('^[ a-z\u0621-\u063a\u0640-\u064a]+$', text):
        return

    return text


class safelist(list):
    def get(self, index, default=None):
        try:
            return self.__getitem__(index)
        except IndexError:
            return default


def patch_arabic_text(text):
    text = re.sub(r'\bالي\b', 'اللي', text)  # 4. Диалектное слово "который" (звучит как illi) регулярно пишется двумя способами: с одной или двумя л: الي اللي
    text = text.replace('أ', 'ا')
    text = text.replace('آ', 'ا')
    text = text.replace('اليسا', 'اليس')  # Хак: Заменяем "Алиса" на "Элис", т.к. встречаются ошибки в самих наговорах

    # Диалектные слова, встречаются оба варианта написания.
    # TODO: узнать про их смысл и правильное написание
    text = text.replace('حطي لي', 'حطيلي')
    text = text.replace('شغلي لي', 'شغليلي')
    text = re.sub(r'\bشغلي\b', 'شغليلي', text)

    text = text.replace('لو سمحتي', 'لوسمحتي')
    text = text.replace('اعطني', 'اعطيني')  # как будто бы исправление опечатки

    return text


def patch_arabic_text_pair(text1, text2):
    """
    Применяет "жёсткие" патчи арабского для текстов.
    Можно использовать только применимо к расчёту метрики WER,
    т.к. просто текст сам по себе может поменять значение
    """
    words1 = safelist(text1.split(' '))
    words2 = safelist(text2.split(' '))
    if len(words1) != len(words2):
        # пытаемся приджоинить предлог "и" ("و") к следующему слову
        WAW = 'و'
        i = 0
        while i < len(words1):
            if words1.get(i) == WAW and words2.get(i) != WAW and i < len(words1) - 1:
                words1[i + 1] = WAW + words1[i + 1]
                words1.pop(i)
                continue
            i += 1

        i = 0
        while i < len(words2):
            if words2.get(i) == WAW and words1.get(i) != WAW and i < len(words2) - 1:
                words2[i + 1] = WAW + words2[i + 1]
                words2.pop(i)
                continue
            i += 1

    if len(words1) != len(words2):
        # пытаемся приджоинить предлог "и" ("و") к следующему слову
        MA = 'ما'
        i = 0
        while i < len(words1):
            if words1.get(i) == MA and words2.get(i) != MA and i < len(words1) - 1:
                words1[i + 1] = MA + words1[i + 1]
                words1.pop(i)
                continue
            i += 1

        i = 0
        while i < len(words2):
            if words2.get(i) == MA and words1.get(i) != MA and i < len(words2) - 1:
                words2[i + 1] = MA + words2[i + 1]
                words2.pop(i)
                continue
            i += 1

    # пытаемся приджоинить частичку "ли" ("لي") к предыдущему слову
    LI = 'لي'
    if len(words1) > len(words2):
        i = len(words1) - 1
        while i > 0:
            if words1.get(i) == LI and words2.get(i - 1).endswith(LI):
                words1[i - 1] = words1[i - 1] + LI
                words1.pop(i)
                continue
            i -= 1

    if len(words2) > len(words1):
        i = len(words2) - 1
        while i > 0:
            if words2.get(i) == LI and words1.get(i - 1).endswith(LI):
                words2[i - 1] = words2[i - 1] + LI
                words2.pop(i)
                continue
            i -= 1

    if len(words1) != len(words2):
        return text1, text2

    for i in range(len(words1)):
        for k in range(4):
            if words1[i] != words2[i]:
                # Элис и Алиса нормализуем
                SPOTTER_ALISA = 'اليسا'
                SPOTTER_ALIS = 'اليس'
                if words1[i] == SPOTTER_ALIS and words2[i] == SPOTTER_ALISA:
                    words2[i] = SPOTTER_ALIS
                if words1[i] == SPOTTER_ALISA and words2[i] == SPOTTER_ALIS:
                    words1[i] = SPOTTER_ALIS

                HA = 'ه'
                if words1[i].endswith(HA) and not words2[i].endswith(HA):
                    words1[i] = words1[i][:-1]
                if words2[i].endswith(HA) and not words1[i].endswith(HA):
                    words2[i] = words2[i][:-1]

                TA_MARBUTA = 'ة'
                if words1[i].endswith(TA_MARBUTA) and not words2[i].endswith(TA_MARBUTA):
                    words1[i] = words1[i][:-1]
                if words2[i].endswith(TA_MARBUTA) and not words1[i].endswith(TA_MARBUTA):
                    words2[i] = words2[i][:-1]

                ALEF_MAKSURA = 'ى'
                if words1[i].endswith(ALEF_MAKSURA) and not words2[i].endswith(ALEF_MAKSURA):
                    words1[i] = words1[i][:-1]
                if words2[i].endswith(ALEF_MAKSURA) and not words1[i].endswith(ALEF_MAKSURA):
                    words2[i] = words2[i][:-1]

                YA = 'ي'
                if words1[i].endswith(YA) and not words2[i].endswith(YA):
                    words1[i] = words1[i][:-1]
                if words2[i].endswith(YA) and not words1[i].endswith(YA):
                    words2[i] = words2[i][:-1]

    return ' '.join(words1), ' '.join(words2)


def main(in1, in2, in3, mr_tables, token1=None, token2=None, param1=None, param2=None, html_file=None):
    metrics = defaultdict(dict)
    metrics['raw'] = {'Cor': 0, 'Ins': 0, 'Del': 0, 'Sub': 0}
    metrics['normalized'] = {'Cor': 0, 'Ins': 0, 'Del': 0, 'Sub': 0}
    metrics['patched'] = {'Cor': 0, 'Ins': 0, 'Del': 0, 'Sub': 0}

    metrics['words_patched'] = {'Cor': Counter(), 'Ins': Counter(), 'Del': Counter(), 'Sub': Counter()}

    for idx, item in enumerate(in1):
        item['text_new_normalized'] = normalize(item['text_new'])
        item['text_old_normalized'] = normalize(item['text_old'])

        if not len(item['text_new']) or not len(item['text_old']):
            print('error item {}'.format(item))

        if not item['text_new_normalized'] or not item['text_old_normalized'] or not len(item['text_new_normalized']) or not len(item['text_old_normalized']):
            print('error item {}'.format(item))

        metrics_raw = calculate_metrics(item['text_new'], item['text_old'], {})
        metrics['raw']['Cor'] += metrics_raw['Cor']
        metrics['raw']['Ins'] += metrics_raw['Ins']
        metrics['raw']['Del'] += metrics_raw['Del']
        metrics['raw']['Sub'] += metrics_raw['Sub']
        # item['metrics_raw'] = metrics_raw

        metrics_normalized = calculate_metrics(item['text_new_normalized'], item['text_old_normalized'], {})
        item['wer_normalized'] = metrics_normalized['WER(basic)']
        metrics['normalized']['Cor'] += metrics_normalized['Cor']
        metrics['normalized']['Ins'] += metrics_normalized['Ins']
        metrics['normalized']['Del'] += metrics_normalized['Del']
        metrics['normalized']['Sub'] += metrics_normalized['Sub']
        # item['metrics_normalized'] = metrics_normalized

        item['text_new_patched'], item['text_old_patched'] = patch_arabic_text_pair(patch_arabic_text(item['text_new_normalized']), patch_arabic_text(item['text_old_normalized']))

        metrics_patched = calculate_metrics(item['text_new_patched'], item['text_old_patched'], {})
        metrics['patched']['Cor'] += metrics_patched['Cor']
        metrics['patched']['Ins'] += metrics_patched['Ins']
        metrics['patched']['Del'] += metrics_patched['Del']
        metrics['patched']['Sub'] += metrics_patched['Sub']
        item['wer_patched'] = metrics_patched['WER(basic)']

        item['idx'] = idx
        item['metrics_patched'] = metrics_patched
        metrics['words_patched']['Cor'].update(metrics_patched['Cor_words'])
        metrics['words_patched']['Ins'].update(metrics_patched['Ins_words'])
        metrics['words_patched']['Del'].update(metrics_patched['Del_words'])
        metrics['words_patched']['Sub'].update(metrics_patched['Sub_words'])
        yield item

    print('metrics raw', metrics['raw'])
    print('metrics normalized', metrics['normalized'])
    print('metrics patched', metrics['patched'])
    print('wer raw = ', (metrics['raw']['Sub'] + metrics['raw']['Del'] + metrics['raw']['Ins']) / (metrics['raw']['Sub'] + metrics['raw']['Del'] + metrics['raw']['Cor']))
    print('wer normalized = ', (metrics['normalized']['Sub'] + metrics['normalized']['Del'] + metrics['normalized']['Ins']) / (metrics['normalized']['Sub'] + metrics['normalized']['Del'] + metrics['normalized']['Cor']))
    print('wer patched = ', (metrics['patched']['Sub'] + metrics['patched']['Del'] + metrics['patched']['Ins']) / (metrics['patched']['Sub'] + metrics['patched']['Del'] + metrics['patched']['Cor']))

    # TOP WORDS:
    # metrics['words_patched']['Cor'].most_common()
    # print('Ins', metrics['words_patched']['Ins'].most_common())
    # print('Del', metrics['words_patched']['Del'].most_common())
    # print('Sub', metrics['words_patched']['Sub'].most_common())
