import json
import string
import logging


class ArabicTranscripter(object):
    def __init__(self, arabic_mapping):
        self.arabic_mapping = arabic_mapping
        self.not_found_letters = {}

        self.TASHKILL = {}
        # порядок огласовок, в котором предпочтительно обрабатывать огласовки
        self.TASHKILL[self.ar_char('Shadda')] = 1
        self.TASHKILL[self.ar_char('Sukun')] = 3
        self.TASHKILL[self.ar_char('Fatha')] = 5
        self.TASHKILL[self.ar_char('Damma')] = 5
        self.TASHKILL[self.ar_char('Kasra')] = 5
        self.TASHKILL[self.ar_char('Fatha tanwin')] = 10
        self.TASHKILL[self.ar_char('Damma tanwin')] = 10
        self.TASHKILL[self.ar_char('Kasra tanwin')] = 10
        self.TASHKILL[self.ar_char('Madda')] = 20
        self.TASHKILL[self.ar_char('Hamza')] = 30
        self.TASHKILL[self.ar_char('HamzaA')] = 30
        self.TASHKILL[self.ar_char('HamzaI')] = 30

        self.VOWELS_TASHKILL = [self.ar_char('Fatha'), self.ar_char('Kasra'), self.ar_char('Damma')]

        self.SUN_LETTERS = [
            self.ar_char('Ta'),
            self.ar_char('Tha'),
            self.ar_char('Dal'),
            self.ar_char('Thal'),
            self.ar_char('Ra'),
            self.ar_char('Zay'),
            self.ar_char('Seen'),
            self.ar_char('Sheen'),
            self.ar_char('Saad'),
            self.ar_char('Daad'),
            self.ar_char('Toh'),
            self.ar_char('Thoh'),
            self.ar_char('Lam'),
            self.ar_char('Noon'),
        ]

    def ar_char(self, name):
        """Возвращает арабский символ по его английскому названию"""
        result = [char for (char, data) in self.arabic_mapping.items() if data['name'] == name]
        assert len(result) == 1, f'Ошибка поиска символа. Вместо 1го результата, для {name} нашлось: {result}'
        return result[0]

    def ar_name(self, letter):
        """Возвращает название арабского символа"""
        if letter not in self.arabic_mapping:
            return letter
        return self.arabic_mapping[letter]['name']

    def ph(self, char):
        """Возвращает фонему для арабского символа"""
        return self.arabic_mapping[char]['phoneme']

    def _get_diacritics_groups(self, utterance):
        """
        Возвращает массив из подряд идущих огласовок
        формат: [[позиция первой огласовки; позиция последней огласовки]]
        """
        diacritics_start = None
        diacritics_groups = []
        for i, c in enumerate(utterance):
            if diacritics_start is None and c in self.TASHKILL.keys():
                diacritics_start = i
            if diacritics_start is not None and c not in self.TASHKILL.keys():
                diacritics_groups.append([diacritics_start, i - 1])
                diacritics_start = None
        if diacritics_start is not None:
            diacritics_groups.append([diacritics_start, len(utterance) - 1])
        return diacritics_groups

    def sort_diacritics(self, utterance):
        diacritics_groups = self._get_diacritics_groups(utterance)
        logging.info(f'diacritics_groups: {diacritics_groups}')
        for (begin, end) in diacritics_groups:
            if end > begin:
                # сортировка диакритики по порядку
                original_order_diacritics = utterance[begin:end+1]
                sorted_diacritics_list = sorted(list(original_order_diacritics), key=lambda x: self.TASHKILL[x])
                sorted_order_diacritics = ''.join(sorted_diacritics_list)
                if original_order_diacritics != sorted_order_diacritics:
                    logging.info('for group ({begin}, {end}), original order: {orig_order}, new_order: {new_order}'.format(
                        begin=begin, end=end,
                        orig_order=[ord(x) for x in original_order_diacritics],
                        new_order=[ord(x) for x in sorted_order_diacritics],
                    ))
                    utterance = utterance[:begin] + sorted_order_diacritics + utterance[end+1:]

        # в лигатуре Лям-Алеф шадда ставится после Алефа —> переставляем после Лям
        sequence_LAW = self.ar_char('Lam') + self.ar_char('Alef') + self.ar_char('Shadda')
        sequence_LWA = self.ar_char('Lam') + self.ar_char('Shadda') + self.ar_char('Alef')
        utterance = utterance.replace(sequence_LAW, sequence_LWA)

        return utterance

    def transcript(self, utterance):
        """
        Транслитерирует арабский текст в английские фонемы
        A fully accurate transcription may not be necessary for native Arabic speakers, as they would be able to pronounce names and sentences correctly anyway, but it can be very useful for those not fully familiar with spoken Arabic and who are familiar with the Roman alphabet. An accurate transliteration serves as a valuable stepping stone for learning, pronouncing correctly, and distinguishing phonemes. It is a useful tool for anyone who is familiar with the sounds of Arabic but not fully conversant in the language.
        One criticism is that a fully accurate system would require special learning that most do not have to actually pronounce names correctly, and that with a lack of a universal romanization system they will not be pronounced correctly by non-native speakers anyway. The precision will be lost if special characters are not replicated and if a reader is not familiar with Arabic pronunciation.

        https://en.wikipedia.org/wiki/Buckwalter_transliteration
        """

        logging.info('original utterance:')
        logging.info(utterance + ' {}'.format([ord(x) for x in utterance]))

        utterance = self.sort_diacritics(utterance)
        logging.info('sort_diacritics utterance:')
        logging.info(utterance + ' {}'.format([ord(x) for x in utterance]))
        logging.info('')

        utterance_translated = ''

        for i, char in enumerate(utterance):
            prev_char = utterance[i - 1] if i > 0 else None
            # prev_prev_char = utterance[i - 2] if i > 1 else None
            next_char = utterance[i + 1] if i + 1 < len(utterance) else None

            if ('А' <= char <= 'Я') or ('а' <= char <= 'я') or ('A' <= char <= 'Z') or ('a' <= char <= 'z') or ('0' <= char <= '9'):
                utterance_translated += char
            elif char in string.punctuation or char in [' ', '\n', '\t', '—', '–', '«', '»', '“', '”', '…', '']:
                utterance_translated += char
            elif char in [self.ar_char('Ya'), self.ar_char('Ya Farsi')]:
                if prev_char == self.ar_char('Kasra') and next_char not in self.VOWELS_TASHKILL:
                    utterance_translated += 'i'
                else:
                    utterance_translated += self.ph(char)
                # utterance_translated += 'i' if i > 0 and utterance[i - 1] == self.ar_char('Kasra') else self.ph(char)
            elif char == self.ar_char('Ta Marbuta'):
                if next_char:
                    utterance_translated += 'at'
                else:
                    utterance_translated += self.ph(char)
            elif char == self.ar_char('Wow'):
                if prev_char == self.ar_char('Damma') and next_char not in self.VOWELS_TASHKILL:
                    utterance_translated += 'u'
                else:
                    utterance_translated += self.ph(char)
                # utterance_translated += 'u' if i > 0 and utterance[i - 1] == self.ar_char('Damma') else self.ph(char)
            elif char == self.ar_char('Alef'):
                if prev_char == self.ar_char('Fatha') and next_char not in self.VOWELS_TASHKILL:
                    utterance_translated += 'a'
                else:
                    utterance_translated += self.ph(char)
                # utterance_translated += 'a' if i > 0 and utterance[i - 1] == self.ar_char('Fatha') else self.ph(char)
            elif char == self.ar_char('Alef with hamza') and next_char == self.ar_char('Damma'):
                # слово начинается на длинное У
                utterance_translated += 'u'
            elif char == self.ar_char('Lam'):
                if i > 0 and i + 1 < len(utterance) and utterance[i - 1] in [self.ar_char('Alef with wasla'), self.ar_char('Alef')]:
                    # артикль Аль
                    # TODO: проверить, что после Лям есть вообще символ.
                    # TODO: проверить, что перед Алеф ничего нет или пробел
                    next_char = utterance[i + 1]
                    if next_char in self.SUN_LETTERS:
                        utterance_translated += self.ph(next_char) + '-'
                    else:
                        utterance_translated += self.ph(char) + '-'
                else:
                    utterance_translated += self.ph(char)
            elif char == self.ar_char('Shadda'):
                # удвоение предыдущего символа
                current_char_index = i - 1
                while current_char_index > 0 and utterance[current_char_index] in self.TASHKILL.keys():
                    current_char_index -= 1

                if utterance[current_char_index] not in self.TASHKILL.keys() and utterance[current_char_index] in self.arabic_mapping:
                    utterance_translated += self.ph(utterance[current_char_index])
                else:
                    # шадда над странным символов, которого нет в маппинге. Ничего не делаем
                    pass
            elif char in self.arabic_mapping:
                utterance_translated += self.ph(char)
            else:
                # логирование символов, которые не нашлись
                if char in self.not_found_letters:
                    self.not_found_letters[char]['cnt'] += 1
                else:
                    self.not_found_letters[char] = {
                        'num': ord(char),
                        'hex': hex(ord(char)),
                        'name': 'XXXXXXXX',
                        'phoneme': 'kkkkkkkk',
                        'cnt': 1
                    }

            logging.info(f'step {i}: "{utterance_translated}"')
            logging.info('{} {} {} {}'.format(utterance[i], ord(utterance[i]), hex(ord(utterance[i])), self.ar_name(utterance[i])))

        if len(self.not_found_letters) > 0:
            logging.info(f'\n!!!!!! not_found_count: {len(self.not_found_letters)}\n')

            # with open('not_found_letters.json', 'w') as f:
            #     json.dump(sorted(self.not_found_letters.items(), key=lambda x: -x[1]['cnt']), f, indent=4, ensure_ascii=False)

        return utterance_translated
