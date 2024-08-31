# -*-coding: utf8 -*-
#!/usr/bin/env python

import random
from utils.nirvana.op_caller import call_as_operation
from phrases import RANDOM, BROKEN, EMPTY

def create_message(text):
    return {
        "message": {
            "cards": [
                {
                    "text": text,
                    "type": "simple_text"
                }
            ]},
        "type": "assistant"
        }


def get_alice_messages(data):
    alice_messages = []
    for item in data:
        for message in item.get('dialog'):
            if message.get('type') == "assistant":
                alice_messages.append(message)
    return alice_messages


def make_hp_data(data, alice_messages, empty_share, broken_share, dura_share, last_share):
    empty_border = empty_share
    broken_border = empty_share + broken_share
    dura_border = empty_share + broken_share + dura_share

    for item in data:
        rn = random.random()
        if rn >= dura_border:
            hp_message = random.choice(alice_messages)
        else:
            if rn < empty_border:
                text = random.choice(list(EMPTY))
            elif rn >= empty_border and rn < broken_border:
                text = random.choice(list(BROKEN))
            else:
                text = random.choice(list(RANDOM))
            hp_message = create_message(text)

        rn = random.random()
        if rn > last_share and item.get('dialog') and len(item['dialog']) > 3:
            item.get('dialog')[-3] = hp_message
        elif item.get('dialog') and len(item['dialog']) > 0:
            item['dialog'][-1] = hp_message
        else:
            item['dialog'] = [hp_message]

    return data

def main(data, empty_share=0., broken_share=0., dura_share=0., last_share=0.9):
    """
    Для сессий из входной корзины генерирует json с плохими ответами для отрисовки дивных карточек.
    С вероятностью (1 - empty_share - broken_share - dura_share) подставляет ответ Алисы на случайный запрос из корзины.
    :param empty_share: доля пустых ответов [0, 1]
    :param broken_share: доля ответов "Я сломалась" [0, 1]
    :param dura_share: доля ответов не в тему из фиксированной корзины [0, 1]
    :param last_share: доля плохих ответов в последней реплике сессии (1 - last_share плохих ответов в контексте)
    :return: json в том же формате, что вход
    """
    alice_messages = get_alice_messages(data)
    hp_data = make_hp_data(data, alice_messages, empty_share, broken_share, dura_share, last_share)

    return hp_data


if __name__ == '__main__':
    call_as_operation(
        main,
        input_spec={"data": {'parser':'json'}}
    )
