from __future__ import unicode_literals

import json
import logging
import random
import pandas as pd
from enum import Enum
from datetime import date

from flask import Flask, request

from alisa import Alisa, Dialog

app = Flask(__name__)

logging.basicConfig(level=logging.DEBUG)

signs = ['♈ Овен', '♉ Телец', '♊ Близнецы', '♋ Рак', '♌ Лев', '♍ Дева', '♎ Весы', '♏ Скорпион', '♐ Стрелец',
         '♑ Козерог', '♒ Водолей', '♓ Рыбы']


class Sign(Enum):
    ARIES = 'aries'
    TAURUS = 'taurus'
    GEMINI = 'gemini'
    CANCER = 'cancer'
    LEO = 'leo'
    MAID = 'maid'
    SCALES = 'scales'
    SCORPIO = 'scorpio'
    SAGITTARIUS = 'sagittarius'
    CAPRICORN = 'capricorn'
    AQUARIUS = 'aquarius'
    PISCES = 'pisces'
    GENERAL = 'general'


class NeuroHoroscopeDialog(Dialog):

    def handle_dialog(self, alisa: Alisa):
        sign = alisa.get_user_state_object('sign')
        reset_sign = alisa.get_button_payload_value('reset_sign') or alisa.has_intent('ANOTHER_SIGN')
        if sign and not reset_sign:
            return self.tell_horoscope_by_sign(alisa, Sign(sign))

        return super().handle_dialog(alisa)

    def tell_horoscope_by_sign(self, alisa: Alisa, sign: Sign):
        alisa.tts_with_text("Ваш гороскоп на сегодня: \n")
        alisa.tts("sil<[300]>")
        alisa.tts_with_text(self.get_horoscope(sign))

        alisa.suggest(self.one_of(['Другой знак']), 'request_sign', payload={'reset_sign': True})
        alisa.voice_button(self.on_intent('ANOTHER_SIGN'), 'request_sign')

        if not alisa.get_user_state_object('sign'):
            alisa.suggest(self.one_of(['Запомнить знак']), 'save_user_sign', payload={'sign': sign.value})
            alisa.tts("sil<[300]>")
            alisa.tts("Хотите услышать про другой знак или запомнить ваш?")
            alisa.voice_button(self.on_intent('REMEMBER_SIGN'), 'save_user_sign')
            alisa.add_to_session_state('prev_sign', sign.value)

    def handle_morning_show(self, alisa):
        alisa.show_episode(text=self.get_horoscope(Sign.GENERAL))

    def help_message(self, alisa):
        alisa.tts_with_text(
            "У умею рассказывать гороскоп по вашему знаку зодиака. "
            "Гороскопы сгенерированы нейросетью на текстах Сорокина и Пелевина. \n")
        self.request_sign(alisa)

    def help(self, alisa):
        self.help_message(alisa)

    def what_you_can_do(self, alisa):
        self.help_message(alisa)

    def greetings(self, alisa: Alisa):
        alisa.tts_with_text(
            "Приветствую тебя в нейрогороскопе. Гороскопы сгенерированы нейросетью на текстах Сорокина и Пелевина. \n")
        self.request_sign(alisa)

    def request_sign(self, alisa: Alisa):
        alisa.tts_with_text("Для какого знака зодиака рассказать гороскоп?")
        alisa.voice_button(self.on_intent('SIGN'), 'tell_user_sign')
        alisa.update_user_state('sign', None)

        for sign in random.sample(signs, 4):
            alisa.suggest(sign, 'tell_user_sign')

    def tell_user_sign(self, alisa: Alisa):
        sign = Sign(alisa.get_intent_slot_value('SIGN', 'sign'))
        if sign:
            self.tell_horoscope_by_sign(alisa, sign)
        else:
            alisa.tts_with_text("Не распознала знак зодиака, попробуйте еще раз")
            alisa.voice_button(self.on_intent('SIGN'), 'tell_user_sign')

    def save_user_sign(self, alisa: Alisa):
        sign = alisa.get_button_payload_value('sign')
        if not sign:
            sign = alisa.get_session_object('prev_sign')
        alisa.tts_with_text(
            "Запомнила. Теперь каждый раз при входе в навык я буду говорить ваш гороскоп. "
            "Гороскопы обновляются раз в сутки.")
        alisa.update_user_state('sign', sign)
        alisa.end_session()

    def fallback(self, alisa):
        logging.info('FALLBACK: %r', alisa.get_original_utterance())
        alisa.tts_with_text(self.one_of([
            'Простите, я вас не поняла. ',
        ]))

        self.request_sign(alisa)

    def maybe(self, perc, value):
        if random.randint(0, 100) < perc:
            return value
        return ""

    def get_horoscope(self, sign: Sign):
        horo_by_date = self.get_horoscope_by_date(sign, date.today())
        if horo_by_date is None:
            return self.get_random_horoscope(sign)
        else:
            return horo_by_date

    def get_horoscope_by_date(self, sign: Sign, date):
        sign_df = horoscopes[sign]
        if sign_df[sign_df.date == str(date)].empty:
            return None
        return sign_df[sign_df.date == str(date)].sample(n=1)['text'].iloc[0]

    def get_random_horoscope(self, sign: Sign):
        return horoscopes[sign].sample(n=1)['text'].iloc[0]


def read_horoscope(sign: Sign):
    return pd.read_csv('horoscopes/' + sign.value + '.csv')


horoscopes = {sign: read_horoscope(sign) for sign in Sign}
dialog = NeuroHoroscopeDialog()


def handle_dialog_yacloud(request, context):
    logging.info('Request: %r', request)

    response = {
        "version": request['version'],
        "response": {
            "end_session": False
        }
    }

    dialog.handle_dialog(Alisa(request, response))

    logging.info('Response: %r', response)

    return response


@app.route("/", methods=['POST'])
def main():
    logging.info('Request: %r', request.json)

    response = {
        "version": request.json['version'],
        "response": {
            "end_session": False
        }
    }

    dialog.handle_dialog(Alisa(request.json, response))

    logging.info('Response: %r', response)

    return json.dumps(
        response,
        ensure_ascii=False,
        indent=2
    )


if __name__ == '__main__':
    app.run(threaded=True, port=5000)
