# coding: utf-8

import copy
from alice.analytics.utils.json_utils import get_path_str


def get_response_dialog(text, vins_response):
    """
    Возвращает данные для визуализации диалога с Алисой на ПП
    Этот json содержит "дивную вёрстку" (div1/div2) ответов Алисы

    Далее в кубике `render div cards` этот json нужно залить на s3
        и ссылку на json подать на вход рендерилке (alice/acceptance/modules/render_div_cards), которая отрисует html
        и далее снять скриншот (с помощью rotor/selenium/vanadium)
        и далее залить скриншот в MDS
        и далее ссылку на MDS положить action.url (происходит в alice deep parser --mode=general)
    :param str text: текст запроса
    :param dict vins_response:
    :return dict:
    """
    if not vins_response:
        return None

    response = vins_response.get('response')
    if not response:
        return None

    return [
        {
            "message": {
                "text": text
            },
            "type": "user"
        },
        {
            "message": response,
            "type": "assistant"
        }
    ]


def trim_card_variable_data(card_input):
    """
    Удаляет из карточек дивной вёрстки изменяющиеся данные
    :param dict cards:
    :return list:
    """
    card = copy.deepcopy(card_input)
    for button in card.get('buttons', []):
        for directive in button.get('directives', []):
            if directive and 'button_id' in directive.get('payload', {}):
                # для поискового запроса генерится каждый раз button_id
                del directive['payload']['button_id']
    return card


def get_screenshot_hashable(text, vins_response):
    """
    Возвращает объект с данными, по которым считается hashsum для скриншота
    :param str text: текст запроса
    :param dict vins_response:
    :return dict:
    """
    dialog = get_response_dialog(text, vins_response)
    if dialog is None:
        return None

    cards = get_path_str(dialog, '1.message.cards')

    return {
        'query': get_path_str(dialog, '0.message.text'),
        'cards': [trim_card_variable_data(x) for x in cards],
    }
